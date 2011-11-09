/*
 * f_eem.c -- USB CDC Ethernet (EEM) link function driver
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2009 EF Johnson Technologies
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/etherdevice.h>
#include <linux/crc32.h>
#include <linux/usb/android_composite.h>
#if defined(CONFIG_MACH_SAMSUNG)
#include <linux/ip.h>
#include <linux/udp.h>
#endif

#include "u_ether.h"

#define EEM_HLEN 2

/*
 * This function is a "CDC Ethernet Emulation Model" (CDC EEM)
 * Ethernet link.
 */

struct eem_ep_descs {
	struct usb_endpoint_descriptor	*in;
	struct usb_endpoint_descriptor	*out;
};

struct f_eem {
	struct gether			port;
	u8				ctrl_id;

	struct eem_ep_descs		fs;
	struct eem_ep_descs		hs;
};

#if defined(CONFIG_MACH_SAMSUNG)
static struct f_eem *_f_eem;

static u8 hostaddr[ETH_ALEN];

struct _bootp_pkt {		/* BOOTP packet format */
	struct iphdr iph;	/* IP header */
	struct udphdr udph;	/* UDP header */
	u8 op;			/* 1=request, 2=reply */
	u8 htype;		/* HW address type */
	u8 hlen;		/* HW address length */
	u8 hops;		/* Used only by gateways */
	__be32 xid;		/* Transaction ID */
	__be16 secs;		/* Seconds since we started */
	__be16 flags;		/* Just what it says */
	__be32 client_ip;		/* Client's IP address if known */
	__be32 your_ip;		/* Assigned IP address */
	__be32 server_ip;		/* (Next, e.g. NFS) Server's IP address */
	__be32 relay_ip;		/* IP address of BOOTP relay */
	u8 hw_addr[16];		/* Client's HW address */
	u8 serv_name[64];	/* Server host name */
	u8 boot_file[128];	/* Name of boot file */
	u8 exten[312];		/* DHCP options / BOOTP vendor extensions */
};

static u32 your_client_ip;
static unsigned char client_hw_addr[6];

#if 1 // workaround that pc usb driver drop multicat packet
static int multicast_to_unicast(struct sk_buff *skb)
{
	struct ethhdr *eth;
	struct iphdr *iph;

	eth = (struct ethhdr *)skb->data;
	iph = (struct iphdr *)(skb->data + ETH_HLEN);

	if(eth->h_dest[0] == 0x01) // if multicast
	{
		if(your_client_ip)
		{
			memcpy((void*)eth->h_dest, (void*)client_hw_addr, sizeof(client_hw_addr));
		}
	}

	return 0;
}
#endif

void save_bootp_client_ip(struct sk_buff *skb)
{
	struct _bootp_pkt *bootp;
	u32 bootp_client_ip;

	bootp = (struct _bootp_pkt *)(skb->data + ETH_HLEN);

	if(bootp->iph.protocol == 0x11) // UDP
	{
		if(ntohs(bootp->udph.source) == 67) // bootps
		{
			if(bootp->op == 2) // boot reply
			{
				bootp_client_ip = ntohl(bootp->your_ip);

				if((bootp_client_ip & 0xFFFFFF00) == 0xC0A8C800) // 192.168.200.X
				{
					memcpy((void*)client_hw_addr, (void*)bootp->hw_addr, sizeof(client_hw_addr));
					your_client_ip = bootp_client_ip;
					printk("[USB EEM] CLIENT ETH : %02X:%02X:%02X:%02X:%02X:%02X\n",
							client_hw_addr[0], client_hw_addr[1], client_hw_addr[2],
							client_hw_addr[3], client_hw_addr[4], client_hw_addr[5]);
					printk("[USB EEM] DHCP REPLY IP : %08X\n", your_client_ip);
				}
			}
		}
	}

	multicast_to_unicast(skb);
}

#ifdef CONFIG_PROC_FS
#define EEM_PROC_ENTRY "driver/eem"

static int eem_proc_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len;
	char *p = page;

	p += sprintf(p, "%d.%d.%d.%d\n",
			(your_client_ip >> 24) & 0xFF,
			(your_client_ip >> 16) & 0xFF,
			(your_client_ip >> 8) & 0xFF,
			(your_client_ip) & 0xFF);

	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif
#endif

static inline struct f_eem *func_to_eem(struct usb_function *f)
{
	return container_of(f, struct f_eem, port.func);
}

/*-------------------------------------------------------------------------*/

/* interface descriptor: */

static struct usb_interface_descriptor eem_intf __initdata = {
	.bLength =		sizeof eem_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_EEM,
	.bInterfaceProtocol =	USB_CDC_PROTO_EEM,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor eem_fs_in_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor eem_fs_out_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *eem_fs_function[] __initdata = {
	/* CDC EEM control descriptors */
	(struct usb_descriptor_header *) &eem_intf,
	(struct usb_descriptor_header *) &eem_fs_in_desc,
	(struct usb_descriptor_header *) &eem_fs_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor eem_hs_in_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor eem_hs_out_desc __initdata = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *eem_hs_function[] __initdata = {
	/* CDC EEM control descriptors */
	(struct usb_descriptor_header *) &eem_intf,
	(struct usb_descriptor_header *) &eem_hs_in_desc,
	(struct usb_descriptor_header *) &eem_hs_out_desc,
	NULL,
};

/* string descriptors: */

static struct usb_string eem_string_defs[] = {
	[0].s = "CDC Ethernet Emulation Model (EEM)",
	{  } /* end of list */
};

static struct usb_gadget_strings eem_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		eem_string_defs,
};

static struct usb_gadget_strings *eem_strings[] = {
	&eem_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/

static int eem_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	DBG(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
		ctrl->bRequestType, ctrl->bRequest,
		w_value, w_index, w_length);

	/* device either stalls (value < 0) or reports success */
	return value;
}


static int eem_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_eem		*eem = func_to_eem(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct net_device	*net;

	/* we know alt == 0, so this is an activation or a reset */
	if (alt != 0)
		goto fail;

	if (intf == eem->ctrl_id) {

		if (eem->port.in_ep->driver_data) {
			DBG(cdev, "reset eem\n");
			gether_disconnect(&eem->port);
		}

		if (!eem->port.in) {
			DBG(cdev, "init eem\n");
			eem->port.in = ep_choose(cdev->gadget,
					eem->hs.in, eem->fs.in);
			eem->port.out = ep_choose(cdev->gadget,
					eem->hs.out, eem->fs.out);
		}

		/* zlps should not occur because zero-length EEM packets
		 * will be inserted in those cases where they would occur
		 */
		eem->port.is_zlp_ok = 1;
		eem->port.cdc_filter = DEFAULT_FILTER;
		DBG(cdev, "activate eem\n");
#if defined(CONFIG_USB_ANDROID_EEM) && defined(CONFIG_USB_ANDROID_RNDIS)
		net = geem_connect(&eem->port);
#else
		net = gether_connect(&eem->port);
#endif
		if (IS_ERR(net))
			return PTR_ERR(net);
	} else
		goto fail;

	return 0;
fail:
	return -EINVAL;
}

static void eem_disable(struct usb_function *f)
{
	struct f_eem		*eem = func_to_eem(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	DBG(cdev, "eem deactivated\n");

	if (eem->port.in_ep->driver_data)
		gether_disconnect(&eem->port);
}

/*-------------------------------------------------------------------------*/

/* EEM function driver setup/binding */

static int __init
eem_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_eem		*eem = func_to_eem(f);
	int			status;
	struct usb_ep		*ep;

	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	eem->ctrl_id = status;
	eem_intf.bInterfaceNumber = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &eem_fs_in_desc);
	if (!ep)
		goto fail;
	eem->port.in_ep = ep;
	ep->driver_data = cdev;	/* claim */

	ep = usb_ep_autoconfig(cdev->gadget, &eem_fs_out_desc);
	if (!ep)
		goto fail;
	eem->port.out_ep = ep;
	ep->driver_data = cdev;	/* claim */

	status = -ENOMEM;

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(eem_fs_function);
	if (!f->descriptors)
		goto fail;

	eem->fs.in = usb_find_endpoint(eem_fs_function,
			f->descriptors, &eem_fs_in_desc);
	eem->fs.out = usb_find_endpoint(eem_fs_function,
			f->descriptors, &eem_fs_out_desc);

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		eem_hs_in_desc.bEndpointAddress =
				eem_fs_in_desc.bEndpointAddress;
		eem_hs_out_desc.bEndpointAddress =
				eem_fs_out_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(eem_hs_function);
		if (!f->hs_descriptors)
			goto fail;

		eem->hs.in = usb_find_endpoint(eem_hs_function,
				f->hs_descriptors, &eem_hs_in_desc);
		eem->hs.out = usb_find_endpoint(eem_hs_function,
				f->hs_descriptors, &eem_hs_out_desc);
	}

	DBG(cdev, "CDC Ethernet (EEM): %s speed IN/%s OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			eem->port.in_ep->name, eem->port.out_ep->name);
	return 0;

fail:
	if (f->descriptors)
		usb_free_descriptors(f->descriptors);

	/* we might as well release our claims on endpoints */
	if (eem->port.out)
		eem->port.out_ep->driver_data = NULL;
	if (eem->port.in)
		eem->port.in_ep->driver_data = NULL;

	ERROR(cdev, "%s: can't bind, err %d\n", f->name, status);

	return status;
}

static void
eem_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_eem	*eem = func_to_eem(f);

	DBG(c->cdev, "eem unbind\n");

	if (gadget_is_dualspeed(c->cdev->gadget))
		usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);
	kfree(eem);
}

static void eem_cmd_complete(struct usb_ep *ep, struct usb_request *req)
{
}

/*
 * Add the EEM header and ethernet checksum.
 * We currently do not attempt to put multiple ethernet frames
 * into a single USB transfer
 */
static struct sk_buff *eem_wrap(struct gether *port, struct sk_buff *skb)
{
	struct sk_buff	*skb2 = NULL;
	struct usb_ep	*in = port->in_ep;
	int		padlen = 0;
	u16		len = skb->len;

#ifdef CONFIG_MACH_SAMSUNG
	save_bootp_client_ip(skb);
#endif

	if (!skb_cloned(skb)) {
		int headroom = skb_headroom(skb);
		int tailroom = skb_tailroom(skb);

		/* When (len + EEM_HLEN + ETH_FCS_LEN) % in->maxpacket) is 0,
		 * stick two bytes of zero-length EEM packet on the end.
		 */
		if (((len + EEM_HLEN + ETH_FCS_LEN) % in->maxpacket) == 0)
			padlen += 2;

		if ((tailroom >= (ETH_FCS_LEN + padlen)) &&
				(headroom >= EEM_HLEN))
			goto done;
	}

	skb2 = skb_copy_expand(skb, EEM_HLEN, ETH_FCS_LEN + padlen, GFP_ATOMIC);
	dev_kfree_skb_any(skb);
	skb = skb2;
	if (!skb)
		return skb;

done:
	/* use the "no CRC" option */
	put_unaligned_be32(0xdeadbeef, skb_put(skb, 4));

	/* EEM packet header format:
	 * b0..13:	length of ethernet frame
	 * b14:		bmCRC (0 == sentinel CRC)
	 * b15:		bmType (0 == data)
	 */
	len = skb->len;
#if 0 // fixed bug
	put_unaligned_le16((len & 0x3FFF) | BIT(14), skb_push(skb, 2));
#else
	put_unaligned_le16((len & 0x3FFF), skb_push(skb, 2));
#endif

	/* add a zero-length EEM packet, if needed */
	if (padlen)
		put_unaligned_le16(0, skb_put(skb, 2));

	return skb;
}

/*
 * Remove the EEM header.  Note that there can be many EEM packets in a single
 * USB transfer, so we need to break them out and handle them independently.
 */
static int eem_unwrap(struct gether *port,
			struct sk_buff *skb,
			struct sk_buff_head *list)
{
	struct usb_composite_dev	*cdev = port->func.config->cdev;
	int				status = 0;

	do {
		struct sk_buff	*skb2;
		u16		header;
		u16		len = 0;

		if (skb->len < EEM_HLEN) {
			status = -EINVAL;
			DBG(cdev, "invalid EEM header\n");
			goto error;
		}

		/* remove the EEM header */
		header = get_unaligned_le16(skb->data);
		skb_pull(skb, EEM_HLEN);

		/* EEM packet header format:
		 * b0..14:	EEM type dependent (data or command)
		 * b15:		bmType (0 == data, 1 == command)
		 */
		if (header & BIT(15)) {
			struct usb_request	*req = cdev->req;
			u16			bmEEMCmd;

			/* EEM command packet format:
			 * b0..10:	bmEEMCmdParam
			 * b11..13:	bmEEMCmd
			 * b14:		reserved (must be zero)
			 * b15:		bmType (1 == command)
			 */
			if (header & BIT(14))
				continue;

			bmEEMCmd = (header >> 11) & 0x7;
			switch (bmEEMCmd) {
			case 0: /* echo */
				len = header & 0x7FF;
				if (skb->len < len) {
					status = -EOVERFLOW;
					goto error;
				}

				skb2 = skb_clone(skb, GFP_ATOMIC);
				if (unlikely(!skb2)) {
					DBG(cdev, "EEM echo response error\n");
					goto next;
				}
				skb_trim(skb2, len);
				put_unaligned_le16(BIT(15) | BIT(11) | len,
							skb_push(skb2, 2));
				skb_copy_bits(skb, 0, req->buf, skb->len);
				req->length = skb->len;
				req->complete = eem_cmd_complete;
				req->zero = 1;
				if (usb_ep_queue(port->in_ep, req, GFP_ATOMIC))
					DBG(cdev, "echo response queue fail\n");
				break;

			case 1:  /* echo response */
			case 2:  /* suspend hint */
			case 3:  /* response hint */
			case 4:  /* response complete hint */
			case 5:  /* tickle */
			default: /* reserved */
				continue;
			}
		} else {
			u32		crc, crc2;
			struct sk_buff	*skb3;

			/* check for zero-length EEM packet */
			if (header == 0)
				continue;

			/* EEM data packet format:
			 * b0..13:	length of ethernet frame
			 * b14:		bmCRC (0 == sentinel, 1 == calculated)
			 * b15:		bmType (0 == data)
			 */
			len = header & 0x3FFF;
			if ((skb->len < len)
					|| (len < (ETH_HLEN + ETH_FCS_LEN))) {
				status = -EINVAL;
				goto error;
			}

			/* validate CRC */
			crc = get_unaligned_le32(skb->data + len - ETH_FCS_LEN);
			if (header & BIT(14)) {
				crc = get_unaligned_le32(skb->data + len
							- ETH_FCS_LEN);
#if 1 // fixed bug
				crc2 = ~crc32_le(~0,
						skb->data,
						len - ETH_FCS_LEN);
#else
				crc2 = ~crc32_le(~0,
						skb->data,
						skb->len - ETH_FCS_LEN);
#endif
			} else {
				crc = get_unaligned_be32(skb->data + len
							- ETH_FCS_LEN);
				crc2 = 0xdeadbeef;
			}
			if (crc != crc2) {
				DBG(cdev, "invalid EEM CRC\n");
#if 1 // workaround for crc calculation bug of pc usb eem driver
				crc2 = ~crc32_le(~0,
						skb->data,
						len - ETH_FCS_LEN);

				if(crc != crc2)
#endif
				goto next;
			}

			skb2 = skb_clone(skb, GFP_ATOMIC);
			if (unlikely(!skb2)) {
				DBG(cdev, "unable to unframe EEM packet\n");
				continue;
			}
			skb_trim(skb2, len - ETH_FCS_LEN);

			skb3 = skb_copy_expand(skb2,
						NET_IP_ALIGN,
						0,
						GFP_ATOMIC);
			if (unlikely(!skb3)) {
				DBG(cdev, "unable to realign EEM packet\n");
				dev_kfree_skb_any(skb2);
				continue;
			}
			dev_kfree_skb_any(skb2);
			skb_queue_tail(list, skb3);
		}
next:
		skb_pull(skb, len);
	} while (skb->len);

error:
	dev_kfree_skb_any(skb);
	return status;
}

/**
 * eem_bind_config - add CDC Ethernet (EEM) network link to a configuration
 * @c: the configuration to support the network link
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @gether_setup().  Caller is also responsible
 * for calling @gether_cleanup() before module unload.
 */
int __init eem_bind_config(struct usb_configuration *c)
{
	struct f_eem	*eem;
	int		status;

	/* maybe allocate device-global string IDs */
	if (eem_string_defs[0].id == 0) {

		/* control interface label */
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		eem_string_defs[0].id = status;
		eem_intf.iInterface = status;
	}

	/* allocate and initialize one new instance */
	eem = kzalloc(sizeof *eem, GFP_KERNEL);
	if (!eem)
		return -ENOMEM;

	eem->port.cdc_filter = DEFAULT_FILTER;

	eem->port.func.name = "cdc_eem";
	eem->port.func.strings = eem_strings;
	/* descriptors are per-instance copies */
	eem->port.func.bind = eem_bind;
	eem->port.func.unbind = eem_unbind;
	eem->port.func.set_alt = eem_set_alt;
	eem->port.func.setup = eem_setup;
	eem->port.func.disable = eem_disable;
	eem->port.wrap = eem_wrap;
	eem->port.unwrap = eem_unwrap;
	eem->port.header_len = EEM_HLEN;

	status = usb_add_function(c, &eem->port.func);
	if (status)
		kfree(eem);
	return status;
}

#ifdef CONFIG_USB_ANDROID_EEM

static u8 hostaddr[ETH_ALEN];

int eem_function_bind_config(struct usb_configuration *c)
{
	int ret;

	ret = geem_setup(c->cdev->gadget, hostaddr);
	if (ret == 0)
		ret = eem_bind_config(c);
	return ret;
}

static struct android_usb_function eem_function = {
	.name = "eem",
	.bind_config = eem_function_bind_config,
};

static int __init init(void)
{
	printk(KERN_INFO "f_eem init\n");
	android_register_function(&eem_function);
	return 0;
}
module_init(init);

#endif /* CONFIG_USB_ANDROID_EEM */

