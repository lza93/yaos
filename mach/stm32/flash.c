#include <io.h>
#include <kernel/lock.h>
#include <kernel/page.h>
#include <kernel/module.h>
#include <error.h>

#define PER			1
#define STRT			6
#define PG			0

#define BLOCK_SIZE		2048
#define BLOCK_LEN		(BLOCK_SIZE / WORD_SIZE)

#define WRITE_WORD(addr, data)	{ \
	FLASH_WRITE_START(); \
	FLASH_SR |= 0x34; \
	FLASH_WRITE_WORD(addr, data); \
	FLASH_WRITE_END(); \
}

static DEFINE_SPINLOCK(wlock);

static inline int __attribute__((section(".iap"))) flash_erase(void *addr)
{
	FLASH_CR |= (1 << PER);
	FLASH_AR = (unsigned int)addr;
	FLASH_CR |= (1 << STRT);
	while (FLASH_SR & 1) ;
	FLASH_CR &= ~(1 << PER);

	return FLASH_SR;
}

static unsigned int __flash_read(void *addr, void *buf, size_t len)
{
	unsigned int *s = (unsigned int *)addr;
	unsigned int *d = (unsigned int *)buf;

	len /= WORD_SIZE;

	while (len--)
		*d++ = *s++;

	return (unsigned int)s - (unsigned int)addr;
}

static size_t flash_read(struct file *file, void *buf, size_t len)
{
	/* check boundary */

	unsigned int nblock, diff;

	nblock = file->offset;
	diff = __flash_read((void *)nblock, buf, len);

	return diff;
}

static int flash_seek(struct file *file, unsigned int offset, int whence)
{
	unsigned int end;
	struct device *dev;

	switch (whence) {
	case SEEK_SET:
		file->offset = offset;
		break;
	case SEEK_CUR:
		if (file->offset + offset < file->offset)
			return -ERR_RANGE;

		file->offset += offset;
		break;
	case SEEK_END:
		dev = getdev(file->inode->dev);
		end = dev->block_size * dev->nr_blocks - 1;
		end = WORD_BASE(end + dev->base_addr);

		if (end - offset < dev->base_addr)
			return -ERR_RANGE;

		file->offset = end - offset;
		break;
	}

	return 0;
}

static size_t __attribute__((section(".iap")))
__flash_write(void *addr, void *buf, size_t len)
{
	unsigned int *temp, *data, *to;
	unsigned int index, sentinel;

	if ((temp = kmalloc(BLOCK_SIZE)) == NULL)
		return -ERR_ALLOC;

	len  = WORD_BASE(len); /* to prevent underflow */
	data = (unsigned int *)buf;
	to   = (unsigned int *)addr;
	sentinel = 0;

	spin_lock(wlock);
	FLASH_UNLOCK();

	for (index = BLOCK_LEN; len; len -= WORD_SIZE) {
		if (index >= BLOCK_LEN) {
			index = 0;
			sentinel = (unsigned int)
				to - BLOCK_BASE(to, BLOCK_SIZE);
			sentinel /= WORD_SIZE;

			to = (unsigned int *)BLOCK_BASE(to, BLOCK_SIZE);
			__flash_read(to, temp, BLOCK_SIZE);
			flash_erase(to);
		}

		while (sentinel) {
			WRITE_WORD(to, temp[index]);
			if (FLASH_SR & 0x14) goto out;

			to++;
			index++;
			sentinel--;
		}

		WRITE_WORD(to, *data);
		if (FLASH_SR & 0x14) goto out;

		to++;
		data++;
		index++;
	}

	while (index < BLOCK_LEN) {
		WRITE_WORD(to, temp[index]);
		if (FLASH_SR & 0x14) goto out;

		to++;
		index++;
	}

out:
	FLASH_LOCK();
	spin_unlock(wlock);

	kfree(temp);

	if (len || (index != BLOCK_LEN)) {
		unsigned int errcode;

		errcode = FLASH_SR;
		FLASH_SR |= 0x34; /* clear flags */

		return errcode;
	}

	return (unsigned int)to - (unsigned int)addr;
}

static size_t __attribute__((section(".iap")))
flash_write(struct file *file, void *buf, size_t len)
{
	unsigned int nblock, diff;

	nblock = file->offset;
	diff = __flash_write((void *)nblock, buf, len);

	return diff;
}

static struct file_operations ops = {
	.open  = NULL,
	.read  = flash_read,
	.write = flash_write,
	.close = NULL,
	.seek  = flash_seek,
};

#include <kernel/buffer.h>

static int flash_init()
{
	extern char _rom_size, _rom_start, _etext, _data, _ebss;
	struct device *dev;
	unsigned int end;

	/* whole disk of embedded flash memory */
	if (!(dev = mkdev(0, 0, &ops, "efm")))
		return -ERR_RANGE;

	dev->block_size = BLOCK_SIZE;
	dev->nr_blocks = (unsigned int)&_rom_size / BLOCK_SIZE;

	/* partition for file system */
	if (!(dev = mkdev(MAJOR(dev->id), 1, &ops, "efm")))
		return -ERR_RANGE;

	dev->block_size = BLOCK_SIZE;
	dev->base_addr = (unsigned int)&_rom_start + (unsigned int)&_etext +
		((unsigned int)&_ebss - (unsigned int)&_data);
	dev->base_addr = ALIGN_BLOCK(dev->base_addr, BLOCK_SIZE);

	end = (unsigned int)&_rom_start + (unsigned int)&_rom_size;
	dev->nr_blocks = (end - dev->base_addr) / BLOCK_SIZE;

	/* request buffer cache */
	if ((dev->buffer = request_buffer(1, dev->block_size)) == NULL)
		return -ERR_ALLOC;

	mount(dev, "/", "embedfs");

	return 0;
}
MODULE_INIT(flash_init);