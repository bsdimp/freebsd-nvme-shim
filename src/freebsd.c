#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/fs.h>
#include <sys/param.h>

#include "ioctl.h"

#undef ioctl

/*
 * XXX we should likely move this a new nvme file, because we can't include
 * sys/dev/nvme/nvme.h because it conflicts with the Linux definitions.
 */
#define	NVME_PASSTHROUGH_CMD		_IOWR('n', 0, struct nvme_pt_command)
#define	NVME_RESET_CONTROLLER		_IO('n', 1)
#define	NVME_GET_NSID			_IOR('n', 2, struct nvme_get_nsid)
#define	NVME_GET_MAX_XFER_SIZE		_IOR('n', 3, uint64_t)

struct nvme_sgl_descriptor {
	uint64_t address;
	uint32_t length;
	uint8_t reserved[3];
	uint8_t type;
};

_Static_assert(sizeof(struct nvme_sgl_descriptor) == 16, "bad size for nvme_sgl_descriptor");

struct nvme_command {
	/* dword 0 */
	uint8_t opc;		/* opcode */
	uint8_t fuse;		/* fused operation */
	uint16_t cid;		/* command identifier */

	/* dword 1 */
	uint32_t nsid;		/* namespace identifier */

	/* dword 2-3 */
	uint32_t rsvd2;
	uint32_t rsvd3;

	/* dword 4-5 */
	uint64_t mptr;		/* metadata pointer */

	/* dword 6-9 */
	union {
		struct {
			uint64_t prp1;	/* prp entry 1 */
			uint64_t prp2;	/* prp entry 2 */
		};
		struct nvme_sgl_descriptor sgl;
	};

	/* dword 10-15 */
	uint32_t cdw10;		/* command-specific */
	uint32_t cdw11;		/* command-specific */
	uint32_t cdw12;		/* command-specific */
	uint32_t cdw13;		/* command-specific */
	uint32_t cdw14;		/* command-specific */
	uint32_t cdw15;		/* command-specific */
};

_Static_assert(sizeof(struct nvme_command) == 16 * 4, "bad size for nvme_command");

struct nvme_completion {
	/* dword 0 */
	uint32_t		cdw0;	/* command-specific */

	/* dword 1 */
	uint32_t		rsvd1;

	/* dword 2 */
	uint16_t		sqhd;	/* submission queue head pointer */
	uint16_t		sqid;	/* submission queue identifier */

	/* dword 3 */
	uint16_t		cid;	/* command identifier */
	uint16_t		status;
} __aligned(8);	/* riscv: nvme_qpair_process_completions has better code gen */

_Static_assert(sizeof(struct nvme_completion) == 4 * 4, "bad size for nvme_completion");

struct nvme_pt_command {
	struct nvme_command	cmd;
	struct nvme_completion	cpl;
	void *			buf;
	uint32_t		len;
	uint32_t		is_read;
	struct mtx *		driver_lock;
};

struct nvme_get_nsid {
	char		cdev[SPECNAMELEN + 1];
	uint32_t	nsid;
};

int FreeBSDPort_linux_ioctl(int fd, unsigned long icmd, ...)
{
	va_list ap;
	int rv;
	uintptr_t arg;

	va_start(ap, icmd);
	arg = va_arg(ap, uintptr_t);
	va_end(ap);
	switch (icmd) {
	case BLKBSZSET:
	case BLKRRPART:
		rv = 0;
		break;
	case NVME_IOCTL_ID: 
	{
		struct nvme_get_nsid nid;

		rv = ioctl(fd, NVME_GET_NSID, &nid);
		if (rv < 0) {
			rv = -errno;
		} else {
			errno = 0;
			rv = nid.nsid;
		}
		break;
	}
	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD:
	{
		struct nvme_pt_command pt = { 0 };
		struct nvme_command *cmd = &pt.cmd;
		struct nvme_passthru_cmd *lpt = (struct nvme_passthru_cmd *)arg;
		struct nvme_get_nsid nid;

		/*
		 * XXX:
		 * I/O commands go to the namespace fd, while admin go to the
		 * raw this should be enforced here somehow, but I'm not sure
		 * how to do that at this level w/o a performance hit. So we do
		 * this and take the hit...
		 */
		rv = ioctl(fd, NVME_GET_NSID, &nid);
		if (rv < 0) {
			rv = -EINVAL;
			break;
		}
		if (nid.nsid == 0 && icmd == NVME_IOCTL_IO_CMD) {
			fprintf(stderr, "Error: Trying to do I/O commands to the admin fd\n");
			rv = -EINVAL;
			break;
		}
		// XXX Not 100% sure the following is an error from the drive's
		// perspective, but FreeBSD implements passthru commands to the
		// ns fd as an I/O command.
		if (nid.nsid != 0 && icmd == NVME_IOCTL_ADMIN_CMD) {
			fprintf(stderr, "Error: Trying to doadmin commands to the namespace fd\n");
			rv = -EINVAL;
			break;
		}

		/*
		 * Filter out what we don't support
		 */
		if (lpt->metadata != 0 || lpt->metadata_len != 0) {
			fprintf(stderr, "metadata not supported for passthru\n");
			rv = -EINVAL;
			break;
		}

		if (lpt->flags != 0) {
			fprintf(stderr, "Can't set the flags in passthru to %#x\n", lpt->flags);
			rv = -EINVAL;
			break;
		}

		/* Copy all the fields */
		cmd = &pt.cmd;
		cmd->opc = lpt->opcode;
		cmd->fuse = lpt->flags;
		cmd->cid = 0;
		cmd->nsid = htole32(lpt->nsid);
		cmd->rsvd2 = htole32(lpt->cdw2);
		cmd->rsvd3 = htole32(lpt->cdw3);
		cmd->mptr = 0;
		cmd->prp1 = 0;
		cmd->prp2 = 0;
		/*
		 * FreeBSD doesn't allow a no buffer data transer, but linux
		 * does. For the moment just malloc a buffer. Nothing is super
		 * performance sensitve that does this.
		 */
		pt.is_read = (cmd->opc & 0x2) != 0;
		/* XXX do we want to check data_len > 0 && addr == 0 && !is_read? */
		if (lpt->data_len > 0 && lpt->addr == 0) {
			pt.buf = malloc(lpt->data_len);
		} else {
			pt.buf = (void *)lpt->addr;
		}
		pt.len = lpt->data_len;
		cmd->cdw10 = htole32(lpt->cdw10);
		cmd->cdw11 = htole32(lpt->cdw11);
		cmd->cdw12 = htole32(lpt->cdw12);
		cmd->cdw13 = htole32(lpt->cdw13);
		cmd->cdw14 = htole32(lpt->cdw14);
		cmd->cdw15 = htole32(lpt->cdw15);
		// timeout_ms; XXX We can't set this -- can't complain about it either
		rv = ioctl(fd, NVME_PASSTHROUGH_CMD, &pt);
		if (rv == 0)
			lpt->result = pt.cpl.cdw0;
		if (rv < 0)
			rv = -errno;
		if (lpt->data_len > 0 && lpt->addr == 0)
			free(pt.buf);
		break;
	}

	case NVME_IOCTL_RESET:
	case NVME_IOCTL_SUBSYS_RESET:
	case NVME_IOCTL_RESCAN:
	case NVME_IOCTL_ADMIN64_CMD:
	case NVME_IOCTL_IO64_CMD:
	case NVME_URING_CMD_IO:
	case NVME_URING_CMD_IO_VEC:
		fprintf(stderr, "Don't know how to do %#lx yet\n", icmd);
		rv = -1;
		break;
	default:
		rv = ioctl(fd, icmd, arg);
		if (rv != 0) {
			fprintf(stderr, "Oh! %#lx failed\n", icmd);
			rv = -errno;
		}
		break;
	}
	if (rv < 0)
		errno = -rv;
	return (rv);
}

#include <fcntl.h>
#undef open

int FreeBSDPort_open(const char *path, int flags, ...)
{
	va_list ap;
	int rv;
	uintptr_t arg;

	va_start(ap, flags);
	arg = va_arg(ap, uintptr_t);
	va_end(ap);

	if (strncmp(path, "/sys/", 5) == 0 ||
	    strncmp(path, "/proc/", 6) == 0) {
		fprintf(stderr, "Unsupported Linux path %s\n", path);
		errno = ENOENT;
		return (-ENOENT);
	}

	rv = open(path, flags, arg);
	return (rv);
}
