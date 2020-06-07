#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <minix/ds.h>
#include <minix/ioctl.h>
#include <sys/ioc_dfa.h>

/* Function prototypes for the dfa driver. */
static int dfa_open(devminor_t minor, int access, endpoint_t user_endpt);
static int dfa_close(devminor_t minor);
static ssize_t dfa_read(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static ssize_t dfa_write(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static int dfa_ioctl(devminor_t minor, unsigned long request, endpoint_t endpt,
    cp_grant_id_t grant, int flags, endpoint_t user_endpt, cdev_id_t id);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

/* Entry points to the dfa driver. */
static struct chardriver dfa_tab =
{
    .cdr_open	= dfa_open,
    .cdr_close	= dfa_close,
    .cdr_read	= dfa_read,
    .cdr_write  = dfa_write,
    .cdr_ioctl  = dfa_ioctl
};

/* State variable to count the number of times the device has been opened.
Note that this is not the regular type of open counter: it never decreases. */
#define STATE_COUNT 256
static unsigned char current_state;
static unsigned char is_accepting[STATE_COUNT];
static unsigned char transitions[STATE_COUNT][STATE_COUNT];

#define BUFFER_SIZE 0x10000
static unsigned char buffer[BUFFER_SIZE];

static int dfa_open(devminor_t UNUSED(minor), int UNUSED(access),
    endpoint_t UNUSED(user_endpt))
{
    // printf("dfa_open(). Called %d time(s).\n", ++open_counter);
    return OK;
}

static int dfa_close(devminor_t UNUSED(minor))
{
    // printf("dfa_close()\n");
    return OK;
}

static ssize_t dfa_read(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t UNUSED(id))
{
    int ret;
    int pattern;

    pattern = is_accepting[current_state] ? 'Y' : 'N';
    pattern = (pattern << 24) + (pattern << 16) + (pattern << 8) + pattern;

    /* Set bytes for the caller. */
    if ((ret = sys_safememset(endpt, grant, 0, pattern, size)) != OK)
        return ret;

    /* Return the number of bytes read. */
    return size;
}


static ssize_t dfa_write(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t UNUSED(id))
{
    int ret;

    for (size_t start = 0; start < size; start += BUFFER_SIZE) {
        size_t end = start + BUFFER_SIZE;
        if (end > size) {
            end = size;
        }

        size_t length = end - start;
        if ((ret = sys_safecopyfrom(endpt, grant, start, (vir_bytes) buffer, length)) != OK)
            return ret;

        for (size_t i = 0; i < length; i++) {
            current_state = transitions[current_state][buffer[i]];
        }
    }

    return size;
}

static int dfa_ioctl(devminor_t UNUSED(minor), unsigned long request, endpoint_t endpt,
    cp_grant_id_t grant, int UNUSED(flags), endpoint_t user_endpt, cdev_id_t UNUSED(id))
{
    int rc;

    switch(request) {
    case DFAIOCRESET:
        current_state = 0;
        break;

    case DFAIOCADD:
        rc = sys_safecopyfrom(endpt, grant, 0, (vir_bytes) buffer, 3);
        if (rc == OK) {
            transitions[buffer[0]][buffer[1]] = buffer[2];
            current_state = 0;
        }
        break;

    case DFAIOCACCEPT:
        is_accepting[current_state] = 1;
        break;

    case DFAIOCREJECT:
        is_accepting[current_state] = 0;
        break;

    default:
        rc = ENOTTY;
    }

    return rc;
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
    /* Save the state. */
    ds_publish_u32("current_state", current_state, DSF_OVERWRITE);

    ds_publish_mem("is_accepting", is_accepting, STATE_COUNT, DSF_OVERWRITE);
    ds_publish_mem("transitions", (char *) transitions, STATE_COUNT * STATE_COUNT, DSF_OVERWRITE);

    return OK;
}

static int lu_state_restore() {
    /* Restore the state. */
    u32_t value;
    size_t length;

    ds_retrieve_u32("current_state", &value);
    ds_delete_u32("current_state");
    current_state = (unsigned char)value;

    ds_retrieve_mem("is_accepting", is_accepting, &length);
    ds_delete_mem("is_accepting");
    ds_retrieve_mem("transitions", char *) transitions, &length);
    ds_delete_mem("transitions");

    return OK;
}

static void sef_local_startup()
{
    /* Register init callbacks. Use the same function for all event types. */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /* Register live update callbacks. */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
    /* Initialize the dfa driver. */
    int do_announce_driver = TRUE;

    switch(type) {
        case SEF_INIT_FRESH:
            current_state = 0;
            memset(is_accepting, 0, STATE_COUNT);
            memset(transitions, 0, STATE_COUNT * STATE_COUNT);
            // printf("%sHello world!\n", dfa_msg);
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;
            // printf("%sHey, I'm a new version!\n", dfa_msg);
        break;

        case SEF_INIT_RESTART:
            // strncpy(dfa_msg, HELLO_MESSAGE, HELLO_LEN);
            // dfa_msg[HELLO_LEN - 1] = 0;
            // printf("%sHey, I've just been restarted!\n", dfa_msg);
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

int main(void)
{
    /* Perform initialization. */
    sef_local_startup();

    /* Run the main loop. */
    chardriver_task(&dfa_tab);
    return OK;
}
