/*
 * Virtio watchdog Support
 *
 * Authors:
 *    Huazhiqiang <huazq12@163.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#ifndef QEMU_VIRTIO_WATCHDOG_H
#define QEMU_VIRTIO_WATCHDOG_H

#include "standard-headers/linux/virtio_watchdog.h"
#include "hw/virtio/virtio.h"
#include "qom/object.h"


#define DEBUG_VIRTIO_WATCHDOG 0

#define DPRINTF(fmt, ...) \
do { \
    if (DEBUG_VIRTIO_WATCHDOG) { \
        fprintf(stderr, "virtio_watchdog: " fmt, ##__VA_ARGS__); \
    } \
} while (0)

typedef struct virtio_watchdog_event VirtIOWatchdogEvent;

#define TYPE_VIRTIO_WATCHDOG "virtio-watchdog-device"
OBJECT_DECLARE_SIMPLE_TYPE(VirtIOWatchdog, VIRTIO_WATCHDOG)
#define VIRTIO_WATCHDOG_GET_PARENT_CLASS(obj) \
        OBJECT_GET_PARENT_CLASS(obj, TYPE_VIRTIO_WATCHDOG)


struct VirtIOWatchdog {
    VirtIODevice parent_obj;

    VirtQueue *vq;             /*start/stop/heartbeat watchdog timer by ctrl_vq*/
    
    size_t config_size;

    uint32_t timeout;           /*watchdog timeout*/

    int enabled;                /* If true, watchdog is enabled. */

    int reboot_enabled;         /* "Reboot" on timer expiry.  The real action
                                 * performed depends on the -watchdog-action
                                 * param passed on QEMU command line.
                                 */

    QEMUTimer *timer;           /*watchdog timer. */
};


#endif /* QEMU_VIRTIO_WATCHDOG_H */

