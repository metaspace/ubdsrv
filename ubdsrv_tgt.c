#include "ubdsrv.h"

static struct ubdsrv_tgt_type *tgt_list[UBDSRV_TGT_TYPE_MAX] = {};

int ubdsrv_register_tgt_type(struct ubdsrv_tgt_type *type)
{
	if (type->type < UBDSRV_TGT_TYPE_MAX && !tgt_list[type->type]) {
		tgt_list[type->type] = type;
		return 0;
	}

	die("usbsrv: target %s/%d can't be registered\n",
			type->name, type->type);
	return -1;
}

int ubdsrv_tgt_init(struct ubdsrv_tgt_info *tgt, char *type, int argc, char
		*argv[])
{
	int i;

	if (type == NULL)
		return -1;

	for (i = 0; i < UBDSRV_TGT_TYPE_MAX; i++) {
		const struct ubdsrv_tgt_type  *ops = tgt_list[i];

		if (strcmp(ops->name, type))
			continue;

		if (!ops->init_tgt(tgt, i, argc, argv)) {
			tgt->ops = ops;
			return 0;
		}
	}

	return -1;
}

/*
 * Called in ubd daemon process context, and before creating per-queue
 * thread context
 */
int ubdsrv_prepare_target(struct ubdsrv_tgt_info *tgt, struct ubdsrv_dev *dev)
{
	struct ubdsrv_ctrl_dev *cdev = container_of(tgt,
			struct ubdsrv_ctrl_dev, tgt);

	if (tgt->ops->prepare_target)
		return tgt->ops->prepare_target(tgt, dev);

	cdev->shm_offset += snprintf(cdev->shm_addr + cdev->shm_offset,
		UBDSRV_SHM_SIZE - cdev->shm_offset,
		"target type: %s\n", tgt->ops->name);

	return 0;
}

void ubdsrv_for_each_tgt_type(void (*handle_tgt_type)(unsigned idx,
			const struct ubdsrv_tgt_type *type, void *data),
		void *data)
{
	int i;

	for (i = 0; i < UBDSRV_TGT_TYPE_MAX; i++) {
		int len;

                const struct ubdsrv_tgt_type  *type = tgt_list[i];

		if (!type)
			continue;
		handle_tgt_type(i, type, data);
	}
}

static inline void ubdsrv_tgt_exit(struct ubdsrv_tgt_info *tgt)
{
	int i;

	for (i = 1; i < tgt->nr_fds; i++)
		close(tgt->fds[i]);
}

void ubdsrv_tgt_deinit(struct ubdsrv_tgt_info *tgt, struct ubdsrv_dev *dev)
{
	ubdsrv_tgt_exit(tgt);

	if (tgt->ops->deinit_tgt)
		tgt->ops->deinit_tgt(tgt, dev);
}
