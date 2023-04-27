/*
 * Virtio Watchdog device
 *
 * Authors:
 *    Huazhiqiang <huazq12@163.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/qdev-properties.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-bus.h"
#include "hw/virtio/virtio-pci.h"
#include "hw/virtio/virtio-watchdog.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qom/object.h"

typedef struct VirtIOWatchdogPCI VirtIOWatchdogPCI;

/*
 * virtio-watchdog-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_WATCHDOG_PCI "virtio-watchdog-pci"
DECLARE_INSTANCE_CHECKER(VirtIOWatchdogPCI, VIRTIO_WATCHDOG_PCI,
                         TYPE_VIRTIO_WATCHDOG_PCI)

struct VirtIOWatchdogPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOWatchdog vdev;
};

/*
 * does watchdog suppurt msi interrupt(nvectors)?
 */
static Property virtio_watchdog_pci_properties[] = {
    DEFINE_PROP_BIT("ioeventfd", VirtIOPCIProxy, flags,
                    VIRTIO_PCI_FLAG_USE_IOEVENTFD_BIT, true),
    DEFINE_PROP_UINT32("vectors", VirtIOPCIProxy, nvectors, 2),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_watchdog_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOWatchdogPCI *vwatchdog = VIRTIO_WATCHDOG_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&vwatchdog->vdev);

    /*default use pci modern mode*/
    virtio_pci_force_virtio_1(vpci_dev);
    if (!qdev_realize(vdev, BUS(&vpci_dev->bus), errp)) {
        return;
    }
}

static void virtio_watchdog_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->realize = virtio_watchdog_pci_realize;
    set_bit(DEVICE_CATEGORY_WATCHDOG, dc->categories);

    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_WATCHDOG;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
    device_class_set_props(dc, virtio_watchdog_pci_properties);
}

static void virtio_watchdog_initfn(Object *obj)
{
    VirtIOWatchdogPCI *dev = VIRTIO_WATCHDOG_PCI(obj);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_WATCHDOG);
}

static const VirtioPCIDeviceTypeInfo virtio_watchdog_pci_info = {
    .generic_name  = TYPE_VIRTIO_WATCHDOG_PCI,
    .instance_size = sizeof(VirtIOWatchdogPCI),
    .instance_init = virtio_watchdog_initfn,
    .class_init    = virtio_watchdog_pci_class_init,
};

static void virtio_watchdog_pci_register_types(void)
{
    virtio_pci_types_register(&virtio_watchdog_pci_info);
}
type_init(virtio_watchdog_pci_register_types)

