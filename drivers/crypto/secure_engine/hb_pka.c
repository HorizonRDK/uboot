/*
 * (c) Copyright 2019.10.25
 */

#include <hb_pka.h>
#include <stdarg.h>
#include <stdio.h>
#include <hb_spacc.h>
#include <common.h>
#include <asm/arch/hb_reg.h>
#include <linux/string.h>

#define CRYPTO_OK			(0)
#define CRYPTO_FAILED		  (-1)
#define CRYPTO_INPROGRESS		(-2)
#define CRYPTO_INVALID_HANDLE	  (-3)
#define CRYPTO_INVALID_CONTEXT	 (-4)
#define CRYPTO_INVALID_SIZE	  (-5)
#define CRYPTO_NOT_INITIALIZED	 (-6)
#define CRYPTO_NO_MEM		  (-7)
#define CRYPTO_INVALID_ALG	   (-8)
#define CRYPTO_INVALID_KEY_SIZE	(-9)
#define CRYPTO_INVALID_ARGUMENT	(-10)

#define PKA_RC_BUSY	  31
#define PKA_RC_IRQ	   30
#define PKA_RC_WR_PENDING  29
#define PKA_RC_ZERO	  28
#define PKA_RC_REASON	16
#define PKA_RC_REASON_BITS  8

#define PKA_STAT_IRQ 30

enum {
	PKA_OPERAND_A,
	PKA_OPERAND_B,
	PKA_OPERAND_C,
	PKA_OPERAND_D,
	PKA_OPERAND_MAX
};

enum {
	PKA_CTRL = 0,
	PKA_ENTRY,
	PKA_RC,
	PKA_BUILD_CONF,
	PKA_F_STACK,
	PKA_INST_SINCE_GO,
	PKA_P_STACK,
	PKA_CONF,
	PKA_STATUS,
	PKA_FLAGS,
	PKA_WATCHDOG,
	PKA_CYCLES_SINCE_GO,
	PKA_INDEX_I,
	PKA_INDEX_J,
	PKA_INDEX_K,
	PKA_INDEX_L,
	PKA_IRQ_EN,
	PKA_DTA_JUMP,
	PKA_LFSR_SEED,

	PKA_BANK_SWITCH_A = 20,
	PKA_BANK_SWITCH_B,
	PKA_BANK_SWITCH_C,
	PKA_BANK_SWITCH_D,

	PKA_OPERAND_A_BASE = 0x100,
	PKA_OPERAND_B_BASE = 0x200,
	PKA_OPERAND_C_BASE = 0x300,
	PKA_OPERAND_D_BASE = 0x400,

	/* F/W base for old cores */
	PKA_FIRMWARE_BASE = 0x800,

	/* F/W base for new ("type 2") cores, with fixed RAM/ROM split offset */
	PKA_FIRMWARE_T2_BASE = 0x1000,
	PKA_FIRMWARE_T2_SPLIT = 0x1800
};

struct pka_param {
	unsigned char func[16];	 /* function name */
	unsigned char name[4];	  /* parameter name */
	unsigned int size;		  /* parameter size */
	unsigned char value[256];   /* parameter value */
};

static void *read_va_param(int output, struct pka_param *param,
						   const char *func, va_list * ap)
{
	while (1) {
		const char *name;
		void *data;

		name = va_arg(*ap, char *);

		if (!name)
			return NULL;

		data = va_arg(*ap, void *);

		/* Skip parameters we're not interested in. */
		if ((output && name[0] != '=') || (!output && name[0] == '=')) {
			continue;
		}

		if (name[0] == '=')
			name++;

		if (name[0] == '%') {
			/* Absolute register */
			memcpy((void *) param->func, (void *) "", sizeof param->func);
			memcpy((void *) param->name, (void *) (name + 1),
				   sizeof param->name);
		} else {
			memcpy((void *) param->func, (void *) func,
				   sizeof param->func);
			memcpy((void *) param->name, (void *) name,
				   sizeof param->name);
		}

		return data;
	}
}

static int lookup_param(const struct pka_param *param,
						unsigned *bank, unsigned *index)
{
	/* Absolute operand references. */
	switch (param->name[0]) {
	case 'A':
		*bank = PKA_OPERAND_A;
		break;
	case 'B':
		*bank = PKA_OPERAND_B;
		break;
	case 'C':
		*bank = PKA_OPERAND_C;
		break;
	case 'D':
		*bank = PKA_OPERAND_D;
		break;
	default:
		return -1;
	}

	*index = param->name[1] - '0';

	return 0;
}

static unsigned base_radix(unsigned size)
{
	if (size <= 16)
		return 0;			   /* Error */
	if (size <= 32)
		return 2;
	if (size <= 64)
		return 3;
	if (size <= 128)
		return 4;
	if (size <= 256)
		return 5;
	if (size <= 512)
		return 6;

	return 0;
}

static unsigned page_size(unsigned size)
{
	unsigned ret;

	ret = base_radix(size);
	if (!ret)
		return ret;

	ret = 8 << ret;
	return ret;
}

static int operand_base_offset(unsigned bank,
							   unsigned index, unsigned size)
{
	unsigned pagesize;
	int ret;

	pagesize = page_size(size);
	if (!pagesize)
		return CRYPTO_INVALID_SIZE;

	switch (bank) {
	case PKA_OPERAND_A:
		ret = PKA_OPERAND_A_BASE;
		break;
	case PKA_OPERAND_B:
		ret = PKA_OPERAND_B_BASE;
		break;
	case PKA_OPERAND_C:
		ret = PKA_OPERAND_C_BASE;
		break;
	case PKA_OPERAND_D:
		ret = PKA_OPERAND_D_BASE;
		break;
	default:
		return CRYPTO_INVALID_ARGUMENT;
	}

	return ret + index * (pagesize >> 2);
}

int load_operand(unsigned bank, unsigned index,
				 unsigned size, const uint8_t * data)
{
	uintptr_t opbase, tmp;
	unsigned i, n;
	int rc;

	rc = operand_base_offset(bank, index, size);
	if (rc < 0)
		return rc;

	opbase = (uintptr_t) (PKA_BASE + rc * 4);
	n = size >> 2;

	for (i = 0; i < n; i++) {
		/*
		 * For lengths that are not a multiple of 4, the incomplete word is
		 * at the _start_ of the data buffer, so we must add the remainder.
		 */
		memcpy(&tmp, data + ((n - i - 1) << 2) + (size & 3), 4);
		mmio_write_32(opbase + i * sizeof(unsigned int), tmp);
		/* printf("obase = %lx %lx\n", opbase+i*sizeof(unsigned int), tmp); */
	}

	/* Write the incomplete word, if any. */
	if (size & 3) {
		tmp = 0;
		memcpy((char *) &tmp + sizeof tmp - (size & 3), data, size & 3);
		mmio_write_32(opbase + i * sizeof(unsigned int), tmp);
		i++;
	}

	/* Zero the remainder of the operand. */
	for (n = page_size(size) >> 2; i < n; i++) {
		mmio_write_32(opbase + i * sizeof(unsigned int), 0);
	}

	return 0;
}

static int set_param(struct pka_param *param)
{
	int rc;
	unsigned bank, index;
	rc = lookup_param(param, &bank, &index);
	if (rc < 0)
		return rc;

	rc = load_operand(bank, index, param->size, param->value);

	return rc;
}

static int load_inputs(const char *func, unsigned size, va_list * ap)
{
	struct pka_param param;
	void *data;
	int rc = 0;

	param.size = size;
	while ((data = read_va_param(0, &param, func, ap)) != 0) {
		debug("input oprand = %c%c value size = %ld data addr = %p\n",
			param.name[0], param.name[1], sizeof param.value, data);
		memcpy(param.value, data, sizeof param.value);
		rc = set_param(&param);

		if (rc == -1)
			return -1;
	}

	return 0;
}

static int do_call(const char *func, unsigned size)
{
	int rc = -1;
	uint32_t ctrl, base;
	uintptr_t regbase;
	int flags = 0;

	regbase = (uintptr_t) PKA_BASE;

	if (!memcmp(func, "calc_r_inv", 10))
		rc = 0x11;
	else if (!memcmp(func, "calc_mp", 7))
		rc = 0x10;
	else if (!memcmp(func, "calc_r_sqr", 10))
		rc = 0x12;
	else if (!memcmp(func, "modexp", 6))
		rc = 0x16;

	base = base_radix(size);

	if (!base)
		return CRYPTO_INVALID_SIZE;

	ctrl = base << PKA_CTRL_BASE_RADIX;

	ctrl |=
		(size & (size - 1) ? (size + 3) / 4 : 0) << PKA_CTRL_PARTIAL_RADIX;
	ctrl |= 1ul << PKA_CTRL_GO;

	mmio_write_32(regbase + PKA_INDEX_I * 4, 0);
	mmio_write_32(regbase + PKA_INDEX_J * 4, 0);
	mmio_write_32(regbase + PKA_INDEX_K * 4, 0);
	mmio_write_32(regbase + PKA_INDEX_L * 4, 0);
	mmio_write_32(regbase + PKA_F_STACK * 4, 0);
	mmio_write_32(regbase + PKA_FLAGS * 4, flags);
	mmio_write_32(regbase + PKA_ENTRY * 4, rc);
	mmio_write_32(regbase + PKA_CTRL * 4, ctrl);

	if (rc == -1)
		return rc;

	while (1) {
		uint32_t status = mmio_read_32(regbase + PKA_RC * 4);

		if (status & 1 << PKA_RC_BUSY) {
			continue;
		}
		status = mmio_read_32(regbase + PKA_STATUS * 4);

		if (status & 1 << PKA_STAT_IRQ) {
			mmio_write_32(regbase + PKA_STATUS * 4, 1 << PKA_STAT_IRQ);
		} else {
			printf("polling status is not as we expect %x\n", status);
		}

		return 0;
	}

	return 0;
}

static int unload_operand(unsigned bank, unsigned index,
						  unsigned size, uint8_t * data)
{
	uintptr_t opbase, tmp;
	unsigned i, n;
	int rc;

	rc = operand_base_offset(bank, index, size);
	if (rc < 0)
		return rc;

	opbase = (uintptr_t) (PKA_BASE + rc * 4);
	n = size >> 2;

	for (i = 0; i < n; i++) {
		tmp = mmio_read_32(opbase + i * sizeof(unsigned int));
		memcpy(data + ((n - i - 1) << 2) + (size & 3), &tmp, 4);
	}

	/* Write the incomplete word, if any. */
	if (size & 3) {
		tmp = mmio_read_32(opbase + i * sizeof(unsigned int));
		memcpy(data, (char *) &tmp + sizeof tmp - (size & 3), size & 3);
	}

	return 0;
}

static int get_param(struct pka_param *param)
{
	int rc;
	unsigned bank, index;
	rc = lookup_param(param, &bank, &index);
	if (rc < 0)
		return rc;

	rc = unload_operand(bank, index, param->size, param->value);

	return 0;
}

static int unload_outputs(const char *func, unsigned size, va_list * ap)
{
	struct pka_param param;
	void *data;
	int rc = 0;

	param.size = size;
	while ((data = read_va_param(1, &param, func, ap)) != 0) {
		/* printf("output oprand = %c%c\n", param.name[0], param.name[1]); */
		rc = get_param(&param);

		/* rc = ioctl(fd, PKA_IOC_GETPARAM, &param); */
		if (rc == -1)
			return -1;

		memcpy(data, param.value, size);
	}

	return 0;
}

int elppka_vrun(const char *func, unsigned size, va_list ap)
{
	va_list ap2;
	int rc;

	va_copy(ap2, ap);
	rc = load_inputs(func, size, &ap2);
	va_end(ap2);
	if (rc == -1)
		return -1;

	rc = do_call(func, size);
	if (rc != 0)
		return rc;

	va_copy(ap2, ap);
	rc = unload_outputs(func, size, &ap2);
	va_end(ap2);
	if (rc == -1)
		return -1;

	return 0;
}

static int run_pka(unsigned sz, const char *func, ...)
{
	va_list ap;
	int rc;

	va_start(ap, func);

	rc = elppka_vrun(func, sz, ap);
	va_end(ap);

	if (rc == -1) {
		return 0;
	} else if (rc > 0) {
		return 0;
	}

	return 1;
}

static int do_modexp(unsigned char *dest, const unsigned char *src,
		const unsigned char *key, unsigned char *exp,
		unsigned int sz)
{
	/* Montgomery precomputation. */
	if (!run_pka(sz, "calc_r_inv", "%D0", key, "=%C0", dest, (char *) 0))
		return 0;

	if (!run_pka(sz, "calc_mp", (char *) 0))
		return 0;

	if (!run_pka(sz, "calc_r_sqr", "%D0", key, "%C0", dest, (char *) 0))
		return 0;

	/* Modular exponentiation. */
	if (!run_pka(sz, "modexp",
				 "%A0", src,
				 "%D2", exp, "%D0", key, "=%A0", dest, (char *) 0))
		return 0;

	return 1;
}

#define RSA_FIX_EXPONENT_65537_L 253
#define RSA_FIX_EXPONENT_65537_H 255
#define RSA_FIX_EXPONENT_65537_L_VAL 1
#define RSA_FIX_EXPONENT_65537_H_VAL 1
#define KEY_LENGTH_2048_BY_BYTE 256

int PKA_public_decrypt(int flen, const unsigned char *src,
		const unsigned char *key, unsigned char *dest)
{
	unsigned char e[KEY_LENGTH_2048_BY_BYTE];
	unsigned int ret;

	memset(e, 0, KEY_LENGTH_2048_BY_BYTE);

	/* exponent number is 0x10001 (65537) */
	e[RSA_FIX_EXPONENT_65537_H] = RSA_FIX_EXPONENT_65537_H_VAL;
	e[RSA_FIX_EXPONENT_65537_L] = RSA_FIX_EXPONENT_65537_L_VAL;

	ret = do_modexp(dest, src, key, e, flen);

	return ret;
}

#define PKA_BIG_ENDIAN_CFG	0x4000000
#define PKA_WD_VALUE		100000000

void pka_init(void)
{
	uintptr_t regbase;

	regbase = (uintptr_t) PKA_BASE;

	/* Set a super-huge-looking (but reasonable) watchdog value. */
	mmio_write_32(regbase + PKA_WATCHDOG * 4, PKA_WD_VALUE);

	/* enabe IRQ stat for polling */
	mmio_write_32(regbase + PKA_IRQ_EN * 4, 1 << PKA_IRQ_EN_STAT);

	/* big endian */
	mmio_write_32(regbase + PKA_CONF * 4, PKA_BIG_ENDIAN_CFG);
}

#if PKA_TEST
void pka_test(void)
{
	pka_init();
	printf("==============================================\n");
	pka_rsa_test();
	printf("==============================================\n");
	return;
}
#endif
