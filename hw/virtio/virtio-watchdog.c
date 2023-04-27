/*
 * Virtio Watchdog Device
 *
 * Authors:
 *  Huazhiqiang   <huazq12@163.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/iov.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/virtio/virtio.h"
#include "hw/qdev-properties.h"
#include "hw/virtio/virtio-watchdog.h"
#include "sysemu/runstate.h"
#include "sysemu/watchdog.h"
#include "qom/object_interfaces.h"
#include "trace.h"

#define VIRTIO_WATCHDOG_HEARTBEAT_TIMEOUT_DEFAULT 30
#define VIRTIO_WATCHDOG_HEARTBEAT_TIMEOUT_MIN 10
#define VIRTIO_WATCHDOG_HEARTBEAT_TIMEOUT_MAX 2048

/* This function is called when the watchdog has either been enabled
 * (hence it starts counting down) or has been keep-alived.
 */
static void virtio_watchdog_restart_timer(VirtIOWatchdog *vwdt)
{
    int64_t timeout = VIRTIO_WATCHDOG_HEARTBEAT_TIMEOUT_DEFAULT;

    if (!vwdt->enabled)
        return;

    timer_mod(vwdt->timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + timeout);
}

/* This is called when the guest disables the watchdog. */
static void virtio_watchdog_disable_timer(VirtIOWatchdog *vwdt)
{
    /*del timer*/
    timer_del(vwdt->timer);
}

/* This function is called when the watchdog expires.
 */
static void virtio_watchdog_timer_expired(void *vp)
{
    VirtIOWatchdog *vwdt = vp;
    /* perform action. */
    watchdog_perform_action();
    if (vwdt->reboot_enabled) {
        virtio_watchdog_restart_timer(vwdt);
    }
}

static void virtio_watchdog_handle_input(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(vdev);
    VirtIOWatchdogEvent event;
    VirtQueueElement *elem;
    int len;

    for (;;) {
        elem = virtqueue_pop(vwdt->vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }

        memset(&event, 0, sizeof(event));
        len = iov_to_buf(elem->out_sg, elem->out_num,
                         0, &event, sizeof(event));

        if (event.type == VIRTIO_WATCHDOG_HEATBEAT) {
            virtio_watchdog_restart_timer(vwdt);
        }else if (event.type == VIRTIO_WATCHDOG_ENABLE && !vwdt->enabled) {
            virtio_watchdog_restart_timer(vwdt);
        }else if (event.type == VIRTIO_WATCHDOG_DISABLE && vwdt->enabled) {
            virtio_watchdog_disable_timer(vwdt);
        }
        virtqueue_push(vwdt->vq, elem, len);
        g_free(elem);
    }
    virtio_notify(vdev, vwdt->vq);
    return;
}

static uint64_t virtio_watchdog_get_features(VirtIODevice *vdev, uint64_t f,
                                            Error **errp)
{
    return f;
}

static size_t virtio_watchdog_config_size(VirtIOWatchdog *s)
{
    return sizeof(struct virtio_watchdog_config);
}

static const VMStateDescription vmstate_virtio_watchdog_device = {
    .name = "virtio-watchdog-device",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(timeout, VirtIOWatchdog),
        VMSTATE_INT32(enabled, VirtIOWatchdog),
        VMSTATE_INT32(reboot_enabled, VirtIOWatchdog),
        VMSTATE_TIMER_PTR(timer, VirtIOWatchdog),
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_watchdog_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(dev);

    if (vwdt->timeout > VIRTIO_WATCHDOG_HEARTBEAT_TIMEOUT_MAX) {
        error_setg(errp, "'timeout' parameter must be non-negative, "
                   "and less than 2048");
        return;
    }

    vwdt->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, virtio_watchdog_timer_expired, vwdt);

    virtio_init(vdev, VIRTIO_ID_WATCHDOG, vwdt->config_size);

    /* allocate vq*/
    vwdt->vq = virtio_add_queue(vdev, 8, virtio_watchdog_handle_input);
}

static void virtio_watchdog_device_unrealize(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(dev);

    timer_free(vwdt->timer);
    virtio_del_queue(vdev, 0);
    virtio_cleanup(vdev);
}

static void virtio_watchdog_device_reset(VirtIODevice *vdev)
{
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(vdev);

    if (vwdt->timer) {
        virtio_watchdog_disable_timer(vwdt);
    }

    vwdt->enabled = 0;
    vwdt->reboot_enabled = 1;
}


static const VMStateDescription vmstate_virtio_watchdog = {
    .name = "virtio-watchdog",
    .minimum_version_id = 1,
    .version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_watchdog_get_config(VirtIODevice *vdev, uint8_t *config)
{
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(vdev);
    struct virtio_watchdog_config cfg = {};

    /*
     * Virtio-watchdog device conforms to VIRTIO 1.0 which is always LE,
     * so we can use LE accessors directly.
     */
    stl_le_p(&cfg.timeout, vwdt->timeout);

    memcpy(config, &cfg, vwdt->config_size);
}

static void virtio_watchdog_set_config(VirtIODevice *vdev,
                                      const uint8_t *config_data)
{
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(vdev);
    struct virtio_watchdog_config cfg;

    memcpy(&cfg, config_data, virtio_watchdog_config_size(vwdt));
}

static void virtio_watchdog_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_virtio_watchdog;
    set_bit(DEVICE_CATEGORY_WATCHDOG, dc->categories);
    vdc->realize = virtio_watchdog_device_realize;
    vdc->unrealize = virtio_watchdog_device_unrealize;
    vdc->get_config = virtio_watchdog_get_config;
    vdc->set_config = virtio_watchdog_set_config;
    vdc->get_features = virtio_watchdog_get_features;
    vdc->reset = virtio_watchdog_device_reset;
    vdc->vmsd = &vmstate_virtio_watchdog_device;
}

static void virtio_watchdog_instance_init(Object *obj)
{
    VirtIOWatchdog *vwdt = VIRTIO_WATCHDOG(obj);

    /*
     * The default config_size is sizeof(struct virtio_watchdog_config).
     * Can be overriden with virtio_watchdog_set_config_size.
     */
    vwdt->config_size = sizeof(struct virtio_watchdog_config);
}

static const TypeInfo virtio_watchdog_info = {
    .name = TYPE_VIRTIO_WATCHDOG,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOWatchdog),
    .instance_init = virtio_watchdog_instance_init,
    .class_init = virtio_watchdog_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_watchdog_info);
}

type_init(virtio_register_types)
