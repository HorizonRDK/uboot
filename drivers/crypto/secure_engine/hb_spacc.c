/*
 * (c) Copyright 2019.10.25
 */

#include <hb_spacc.h>
#include <common.h>
#include <asm/arch/hb_bifsd.h>
#include <asm/arch/hb_reg.h>
#include <linux/string.h>

#define x2a_htonl(x) \
({ \
	uint32_t __x = (x); \
	((uint32_t)( \
	(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
	(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
	(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
	(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})

/********** KEY_SZ Bitmasks **********/
#define _SPACC_KEY_SZ_SIZE	 0
#define _SPACC_KEY_SZ_CTX_IDX	8
#define _SPACC_KEY_SZ_CIPHER	 31
#define _SPACC_STATUS_RET_CODE   24
#define SPACC_KEY_SZ_CIPHER	(1UL << _SPACC_KEY_SZ_CIPHER)

#define SPACC_SET_CIPHER_KEY_SZ(z) (((z)<< _SPACC_KEY_SZ_SIZE) | \
	(1UL << _SPACC_KEY_SZ_CIPHER))
#define SPACC_SET_HASH_KEY_SZ(z)   ((z) << _SPACC_KEY_SZ_SIZE)
#define SPACC_SET_KEY_CTX(ctx)   ((ctx) << _SPACC_KEY_SZ_CTX_IDX)

#define SPACC_AUTO_SIZE		(-1)
#define CRYPTO_FAILED		  (-1)
#define CRYPTO_INVALID_BLOCK_ALIGNMENT (-13)
#define CRYPTO_INVALID_MODE	   (int)-14
#define CRYPTO_MEMORY_ERROR	  (-18)

/********** Context Offsets **********/
#define SPACC_CTX_CIPH_KEY   0x04000L
#define SPACC_CTX_HASH_KEY   0x08000L
#define SPACC_CTX_RC4_CTX	0x20000L

#define OP_ENCRYPT	  0
#define OP_DECRYPT	  1

/********* AUX INFO Bitmasks *********/
#define _SPACC_AUX_INFO_DIR	0
#define _SPACC_AUX_INFO_BIT_ALIGN  1
#define _SPACC_AUX_INFO_CBC_CS  16

#define AUX_INFO_SET_DIR(a)	 ((a) << _SPACC_AUX_INFO_DIR)
#define AUX_INFO_SET_BIT_ALIGN(a) ((a) << _SPACC_AUX_INFO_BIT_ALIGN)
#define AUX_INFO_SET_CBC_CS(a)  ((a) << _SPACC_AUX_INFO_CBC_CS)

#define PDU_DMA_ADDR_T		 uint32_t

/********* SW_CTRL Bitmasks ***********/

#define _SPACC_SW_CTRL_ID_0	  0
#define SPACC_SW_CTRL_ID_W	   8
#define SPACC_SW_CTRL_ID_MASK	(0xFF << _SPACC_SW_CTRL_ID_0)
#define SPACC_SW_CTRL_ID_GET(y)	(((y) & SPACC_SW_CTRL_ID_MASK) \
	>> _SPACC_SW_CTRL_ID_0)
#define SPACC_SW_CTRL_ID_SET(id) (((id) & SPACC_SW_CTRL_ID_MASK) \
	>> _SPACC_SW_CTRL_ID_0)

#define _SPACC_SW_CTRL_PRIO	  30
#define SPACC_SW_CTRL_PRIO_MASK	0x3
#define SPACC_SW_CTRL_PRIO_SET(prio) (((prio) & SPACC_SW_CTRL_PRIO_MASK) \
	<< _SPACC_SW_CTRL_PRIO)

#define SPACC_AES_KEY_SIZE 16
#define SPACC_AES_IV_SIZE 16

#define ICV_PT				(1 << 26)

//#define SPACC_DBG_REG	(X2_BIFSPI_BASE + BSPI_SHARE_30_REG)

enum spacc_ret_code {
	SPACC_OK = 0,
	SPACC_ICVFAIL,
	SPACC_MEMERR,
	SPACC_BLOCKERR,
	SPACC_SECERR,
};

typedef struct {
	PDU_DMA_ADDR_T phys;
	uint32_t *virt;

	uint32_t idx, limit, len;
} pdu_ddt;

enum eicvpos {
	IP_ICV_OFFSET = 0,
	IP_ICV_APPEND = 1,
	IP_ICV_IGNORE = 2,
	IP_MAX
};

typedef struct {
	uint32_t
	 enc_mode,				  /* Encription Algorith mode */
	 hash_mode,				  /* HASH Algorith mode */
	 icv_len, icv_offset, op, /* Operation */
	 ctrl,					  /* CTRL shadoe register */
	 first_use,		/* indicates that context just has been initialized/taken */
					/* and this is the first use */
	 pre_aad_sz, post_aad_sz,   /* size of AAD for the latest packet */
	 hkey_sz, ckey_sz;

	unsigned auxinfo_dir, auxinfo_bit_align;
	/* Direction and bit alignment parameters for the AUX_INFO reg */
	unsigned auxinfo_cs_mode;   /* AUX info setting for CBC-CS */

	uint32_t ctx_idx;
	unsigned job_used, job_swid, job_done, job_err, job_secure;
} spacc_job;

typedef struct {
	void *ciph_key;			 /* Memory context to store cipher keys */
	void *hash_key;			 /* Memory context to store hash keys */
	uint32_t *rc4_key;		 /* Memory context to store internal RC4 keys */
	int ref_cnt;			 /* reference count of jobs using this context */
	int ncontig;	    /* number of contexts following related to this one */
} spacc_ctx;

enum ecipher {
	C_NULL = 0,
	C_DES = 1,
	C_AES = 2,
	C_RC4 = 3,
	C_MULTI2 = 4,
	C_KASUMI = 5,
	C_SNOW3G_UEA2 = 6,
	C_ZUC_UEA3 = 7,
	C_CHACHA20 = 8,

	C_MAX
};

enum {
	SPACC_CTRL_CIPH_ALG,
	SPACC_CTRL_CIPH_MODE,
	SPACC_CTRL_HASH_ALG,
	SPACC_CTRL_HASH_MODE,
	SPACC_CTRL_ENCRYPT,
	SPACC_CTRL_CTX_IDX,
	SPACC_CTRL_SEC_KEY,
	SPACC_CTRL_AAD_COPY,
	SPACC_CTRL_ICV_PT,
	SPACC_CTRL_ICV_ENC,
	SPACC_CTRL_ICV_APPEND,
	SPACC_CTRL_KEY_EXP,
	SPACC_CTRL_MSG_BEGIN,
	SPACC_CTRL_MSG_END,
	SPACC_CTRL_MAPSIZE
};

enum eciphermode {
	CM_ECB = 0,
	CM_CBC = 1,
	CM_CTR = 2,
	CM_CCM = 3,
	CM_GCM = 5,
	CM_OFB = 7,
	CM_CFB = 8,
	CM_F8 = 9,
	CM_XTS = 10,

	CM_MAX
};

enum ehashmode {
	HM_RAW = 0,
	HM_SSLMAC = 1,
	HM_HMAC = 2,

	HM_MAX
};

static const uint8_t spacc_ctrl_map[SPACC_CTRL_MAPSIZE] =
	{ 0, 4, 8, 13, 15, 16, 24, 25, 26, 27, 28, 29, 30, 31 };

#define SPACC_CTRL_MASK(field)  (1UL << spacc_ctrl_map[(field)])
#define SPACC_CTRL_SET(field, value)  ((value) << spacc_ctrl_map[(field)])
#define SPACC_GET_STATUS_RET_CODE(s)		(((s) >> _SPACC_STATUS_RET_CODE)&0x7)
#define SPACC_GET_STATUS_SWID(s)			((s)&0xff)
#define SPACC_CRYPTO_OPERATION   1
#define SPACC_HASH_OPERATION   2
#define CRYPTO_AUTHENTICATION_FAILED   (-16)
#define SPACC_AADCOPY_FLAG	0x80000000

/* write a 32-bit word to a given address */
void pdu_io_write32(void *addr, uint32_t val)
{
	*((uint32_t *) addr) = val;
}

/* read a 32-bit word from a given address */
uint32_t pdu_io_read32(void *addr)
{
	uint32_t foo;
	foo = *((uint32_t *) addr);
	return foo;
}

static void put_buf(unsigned char *dst, const unsigned char *src, int off,
					int n, int len)
{
	while (n && (off < len)) {
		dst[off++] = *src++;
		--n;
	}
}

void pdu_from_dev32_little(unsigned char *dst, void *addr_,
						   unsigned int nword)
{
	unsigned char *addr = addr_;
	unsigned int v;

	while (nword--) {
		v = pdu_io_read32(addr);
		addr += 4;
		*dst++ = v & 0xFF;
		v >>= 8;
		*dst++ = v & 0xFF;
		v >>= 8;
		*dst++ = v & 0xFF;
		v >>= 8;
		*dst++ = v & 0xFF;
		v >>= 8;
	}
}

void pdu_to_dev32_little(void *addr_, const unsigned char *src,
						 unsigned int nword)
{
	unsigned char *addr = addr_;
	unsigned int v;

	while (nword--) {
		v = 0;
		v = (v >> 8) | ((uint32_t) *src++ << 24UL);
		v = (v >> 8) | ((uint32_t) *src++ << 24UL);
		v = (v >> 8) | ((uint32_t) *src++ << 24UL);
		v = (v >> 8) | ((uint32_t) *src++ << 24UL);
		pdu_io_write32(addr, v);
		addr += 4;
	}
}

static int spacc_open(spacc_job * sj, int cipher, int hash)
{
	int ret = CRYPTO_OK;
	uint32_t ctrl = 0;
	spacc_job *job = NULL;

	job = sj;

	switch (cipher) {
	case CRYPTO_MODE_AES_CBC:
		ctrl |= SPACC_CTRL_SET(SPACC_CTRL_CIPH_ALG, C_AES);
		ctrl |= SPACC_CTRL_SET(SPACC_CTRL_CIPH_MODE, CM_CBC);
		break;
	default:
		ret = CRYPTO_INVALID_ALG;
		break;
	}

	switch (hash) {
	case CRYPTO_MODE_HASH_SHA256:
		ctrl |= SPACC_CTRL_SET(SPACC_CTRL_HASH_ALG, H_SHA256);
		ctrl |= SPACC_CTRL_SET(SPACC_CTRL_HASH_MODE, HM_RAW);
		break;
	default:
		ret = CRYPTO_INVALID_ALG;
		break;
	}

	job->enc_mode = cipher;
	job->hash_mode = hash;
	job->icv_len = 0;
	job->icv_offset = 0;
	job->op = 0;
	job->ctx_idx = 0;
	job->ctrl = ctrl | SPACC_CTRL_SET(SPACC_CTRL_CTX_IDX, job->ctx_idx);
	job->first_use = 1;
	job->ckey_sz = 0;
	job->hkey_sz = 0;
	job->job_done = 0;
	job->job_swid = 0;

	job->auxinfo_cs_mode = 0;
	job->auxinfo_bit_align = 0;
	job->auxinfo_dir = 0;

	return ret;
}

static int spacc_write_context(spacc_job *job, spacc_ctx *ctx, int op,
							   const unsigned char *key, int ksz,
							   const unsigned char *iv, int ivsz)
{
	int ret = CRYPTO_OK;
	unsigned char buf[64] = { '0' };
	int buflen;

	buflen = sizeof(buf);

	switch (op) {
	case SPACC_CRYPTO_OPERATION:
		switch (job->enc_mode) {
		case CRYPTO_MODE_AES_ECB:
		case CRYPTO_MODE_AES_CBC:
		case CRYPTO_MODE_AES_GCM:
			put_buf(buf, key, 0, ksz, buflen);
			if (iv) {
				unsigned char one[4] = { 0, 0, 0, 1 };
				put_buf(buf, iv, 32, ivsz, buflen);

				if (ivsz == 12
					&& job->enc_mode == CRYPTO_MODE_AES_GCM) {
					put_buf(buf, one, 11 * 4, 4, buflen);
				}
			}
			break;
		case CRYPTO_MODE_NULL:
		default:
			break;
		}
		if (key) {
			job->ckey_sz = SPACC_SET_CIPHER_KEY_SZ(ksz);
			job->hkey_sz = 0;
			job->first_use = 1;
		}

		pdu_to_dev32_little(ctx->ciph_key, buf, buflen >> 2);
		break;

	case SPACC_HASH_OPERATION:
		pdu_from_dev32_little(buf, ctx->hash_key, buflen >> 2);
		pdu_to_dev32_little(ctx->hash_key, buf, buflen >> 2);
		break;

	default:
		ret = CRYPTO_INVALID_MODE;
		break;
	}
	return ret;
}

static int spacc_set_operation(spacc_job *job, int op, uint32_t prot,
							   uint32_t icvcmd, uint32_t icvoff,
							   uint32_t icvsz, uint32_t sec_key)
{
	int ret = CRYPTO_OK;

	if (op == OP_ENCRYPT) {
		job->op = OP_ENCRYPT;
		job->ctrl |= SPACC_CTRL_MASK(SPACC_CTRL_ENCRYPT);
	} else {
		job->op = OP_DECRYPT;
		job->ctrl &= ~SPACC_CTRL_MASK(SPACC_CTRL_ENCRYPT);
	}

	job->icv_len = icvsz;

	switch (icvcmd) {
	case IP_ICV_OFFSET:
		job->icv_offset = icvoff;
		job->ctrl &= ~SPACC_CTRL_MASK(SPACC_CTRL_ICV_APPEND);
		break;

	case IP_ICV_APPEND:
		job->ctrl |= SPACC_CTRL_MASK(SPACC_CTRL_ICV_APPEND);
		break;

	case IP_ICV_IGNORE:
		break;

	default:
		ret = CRYPTO_INVALID_MODE;
		break;
	}
	return ret;
}

static int spacc_set_key_exp(spacc_job *job, spacc_ctx *ctx)
{
	if (!ctx) {
		return CRYPTO_FAILED;
	}

	job->ctrl |= SPACC_CTRL_MASK(SPACC_CTRL_KEY_EXP);
	return CRYPTO_OK;
}

#define MAKEAVECTOR				0

/* 63K, because IP limiation (2^64-1 byte...) */
#define MAX_PROC_LEN			(32*1024)

/* sha256 size is 256 bits-> 32 bytes */
#define SHA256_SIZE	 			32

/* sha256 block size is 512 bits -> 64 bytes */
#define HASH_SET_BLOCK_SIZE		64

#define DDT_SINGLE_INSTANCE_SIZE 4
#define WD_TIMEOUT_VALUE		0x40000
#define CMD0_FIFO_RAISE			0x001
#define WD_TIMEOUT				0x1000
#define FIFO_STAT_EMPTY			0x80000000
#define JOB_IDX					1

uint32_t qsrc[DDT_SINGLE_INSTANCE_SIZE] = { 0 };
uint32_t qdst[DDT_SINGLE_INSTANCE_SIZE] = { 0 };

static int spacc_packet_enqueue(spacc_job *job, int hash,
								uintptr_t src_ddt, uintptr_t dst_ddt,
								uint32_t proc_sz, uint32_t aad_offset,
								uint32_t pre_aad_sz, uint32_t post_aad_sz,
								uint32_t iv_offset)
{
	int ret = CRYPTO_OK, proc_len;
#if MAKEAVECTOR
	int x;
#endif
	int stat;

	qsrc[0] = src_ddt;
	qsrc[1] = proc_sz;
	qsrc[2] = 0;
	qsrc[3] = 0;

	qdst[0] = dst_ddt;
	if (!hash)
		qdst[1] = proc_sz;
	else
		qdst[1] = SHA256_SIZE;
	qdst[2] = 0;
	qdst[3] = 0;

	proc_len = proc_sz;

	if (pre_aad_sz & SPACC_AADCOPY_FLAG) {
		job->ctrl |= SPACC_CTRL_MASK(SPACC_CTRL_AAD_COPY);
		pre_aad_sz &= ~(SPACC_AADCOPY_FLAG);
	} else {
		job->ctrl &= ~SPACC_CTRL_MASK(SPACC_CTRL_AAD_COPY);
	}
	job->pre_aad_sz = pre_aad_sz;
	job->post_aad_sz = post_aad_sz;

	mmio_write_32(SPACC_BASE + SPACC_REG_SRC_PTR, (uintptr_t) qsrc);
	mmio_write_32(SPACC_BASE + SPACC_REG_DST_PTR, (uintptr_t) qdst);

	mmio_write_32(SPACC_BASE + SPACC_REG_PROC_LEN,
				  proc_len - job->post_aad_sz);
	mmio_write_32(SPACC_BASE + SPACC_REG_ICV_LEN, job->icv_len);
	mmio_write_32(SPACC_BASE + SPACC_REG_ICV_OFFSET, job->icv_offset);
	mmio_write_32(SPACC_BASE + SPACC_REG_PRE_AAD_LEN, job->pre_aad_sz);
	mmio_write_32(SPACC_BASE + SPACC_REG_POST_AAD_LEN, job->post_aad_sz);
	mmio_write_32(SPACC_BASE + SPACC_REG_IV_OFFSET, iv_offset);
	mmio_write_32(SPACC_BASE + SPACC_REG_OFFSET, aad_offset);
	mmio_write_32(SPACC_BASE + SPACC_REG_STAT_WD_CTRL, WD_TIMEOUT_VALUE);

	mmio_write_32(SPACC_BASE + SPACC_REG_AUX_INFO,
				  AUX_INFO_SET_DIR(job->auxinfo_dir)
				  | AUX_INFO_SET_BIT_ALIGN(job->auxinfo_bit_align)
				  | AUX_INFO_SET_CBC_CS(job->auxinfo_cs_mode));

	mmio_write_32(SPACC_BASE + SPACC_REG_KEY_SZ,
				  job->ckey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
	mmio_write_32(SPACC_BASE + SPACC_REG_KEY_SZ,
				  job->hkey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
	/* 
	 * write the job ID to the core,
	 * we keep track of it in software now to avoid excessive port I/O
	 */
	mmio_write_32(SPACC_BASE + SPACC_REG_SW_CTRL,
				  SPACC_SW_CTRL_ID_SET(0) | SPACC_SW_CTRL_PRIO_SET(0));

	mmio_write_32(SPACC_BASE + SPACC_REG_CTRL, job->ctrl | ICV_PT);

#if MAKEAVECTOR
	debug("VEC: Initial cipher context\n");
	for (x = 0; x < 64; x += 4) {
		debug("VEC: %08x\n", x2a_htonl((unsigned int)
								  mmio_read_32(SPACC_BASE +
											   SPACC_CTX_CIPH_KEY + x)));
	}
	debug("VEC: END\n");

	debug("VEC: Initial hash context\n");
	for (x = 0; x < 64; x += 4) {
		debug("VEC: %08x\n", x2a_htonl((unsigned int)
								  mmio_read_32(SPACC_BASE +
											   SPACC_CTX_HASH_KEY + x)));
	}
	debug("VEC: END\n");

	debug("VEC: START\n");
	debug("VEC: 00000012  //Opcode\n");
	debug("VEC: %08x  //CTRL register\n",
		 (unsigned int) (mmio_read_32(SPACC_BASE + SPACC_REG_CTRL)));
	debug("VEC: %08x  //source packet length\n",
		 (proc_sz + pre_aad_sz + post_aad_sz));
	debug("VEC: %08x  //ICV length\n", (unsigned int) (job->icv_len));
	debug("VEC: %08lx   //ICV offset\n", (job->icv_offset));
	debug("VEC: %08x  //pre_aad\n", (unsigned int) (pre_aad_sz));
	debug("VEC: %08x  //PROC len\n",
		 (unsigned int) (mmio_read_32(SPACC_BASE + SPACC_REG_PROC_LEN)));
	debug("VEC: %08x  //post_aad\n", (unsigned int) (post_aad_sz));
	debug("VEC: %08x  //IV_OFFSET\n", (unsigned int) (iv_offset));
	debug("VEC: %08x  //AUX register\n",
		 x2a_htonl(mmio_read_32(SPACC_BASE + SPACC_REG_AUX_INFO)));
	debug("VEC: %08x  //cipher key sz\n",
		 x2a_htonl(job->ckey_sz | SPACC_SET_KEY_CTX(job->ctx_idx)));
	debug("VEC: %08x  //hash key sz\n",
		 x2a_htonl(job->hkey_sz | SPACC_SET_KEY_CTX(job->ctx_idx)));
	debug("VEC: %08x  //hash src ptr\n",
		 (unsigned int) mmio_read_32(SPACC_BASE + SPACC_REG_SRC_PTR));
	debug("VEC: %08x  //hash dst ptr\n",
		 (unsigned int) mmio_read_32(SPACC_BASE + SPACC_REG_DST_PTR));
	debug("VEC: END\n");
#endif

	/* Clear an expansion key after the first call */
	if (job->first_use == 1) {
		job->first_use = 0;
		job->ctrl &= ~SPACC_CTRL_MASK(SPACC_CTRL_KEY_EXP);
	}

	stat = mmio_read_32(SPACC_BASE + SPACC_REG_IRQ_STAT);
	if (stat & WD_TIMEOUT)
		mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_STAT, WD_TIMEOUT);
	mmio_write_32(SPACC_BASE + SPACC_REG_STAT_WD_CTRL, WD_TIMEOUT_VALUE);

	while (1) {
		stat = mmio_read_32(SPACC_BASE + SPACC_REG_IRQ_STAT);
		if (stat & CMD0_FIFO_RAISE)
			break;
		if (stat & WD_TIMEOUT) {
			printf("spacc enqueue timeout!!!\n");
			return CRYPTO_FAILED;
		}
	}

	mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_STAT, stat);

	return ret;
}

static int spacc_packet_dequeue(int job_idx)
{
	uint32_t cmdstat, ret = 0, fifo_stat;
	int stat;

	stat = mmio_read_32(SPACC_BASE + SPACC_REG_IRQ_STAT);
	if (stat & WD_TIMEOUT)
		mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_STAT, WD_TIMEOUT);
	mmio_write_32(SPACC_BASE + SPACC_REG_STAT_WD_CTRL, WD_TIMEOUT_VALUE);

	while (1) {
		stat = mmio_read_32(SPACC_BASE + SPACC_REG_IRQ_STAT);
		fifo_stat = mmio_read_32(SPACC_BASE + SPACC_REG_FIFO_STAT);
		if (!(fifo_stat & FIFO_STAT_EMPTY))
			break;

		if (stat & WD_TIMEOUT) {
			printf("spacc dequeue timeout!!!\n");
			return CRYPTO_FAILED;
		}
	}

	/* write the pop register to get the next job */
	mmio_write_32(SPACC_BASE + SPACC_REG_STAT_POP, 1);
	cmdstat = mmio_read_32(SPACC_BASE + SPACC_REG_STATUS);

	/* Check the SWID we pop from stat FIFO is 0*/
	if (SPACC_GET_STATUS_SWID(cmdstat) != 0) {
		printf("SW_ID:%d we dequeue from stat FIFO is wrong\n",
			SPACC_GET_STATUS_SWID(cmdstat));
		return CRYPTO_FAILED;
	}

	/* mark job as done */
	switch (SPACC_GET_STATUS_RET_CODE(cmdstat)) {
	case SPACC_ICVFAIL:
		ret = CRYPTO_AUTHENTICATION_FAILED;
		break;
	case SPACC_MEMERR:
		ret = CRYPTO_MEMORY_ERROR;
		break;
	case SPACC_BLOCKERR:
		ret = CRYPTO_INVALID_BLOCK_ALIGNMENT;
		break;
	case SPACC_SECERR:
		ret = CRYPTO_FAILED;
		break;

	case SPACC_OK:
		ret = CRYPTO_OK;
		break;
	}

	if (ret != CRYPTO_OK)
		printf("%s error, return = %d\n", __FUNCTION__, ret);

	return ret;
}

int spacc_ex(uint64_t src, uint64_t dst, unsigned int src_len,
			 int cipher, int encrypt, int hash, const unsigned char *key,
			 unsigned keylen, const unsigned char *iv, unsigned ivlen)
{
	spacc_job sj;
	spacc_ctx ctx;
	unsigned int curr_len = 0, chunk_len = 0;

	ctx.ciph_key = (void *) (SPACC_BASE + SPACC_CTX_CIPH_KEY);
	ctx.hash_key = (void *) (SPACC_BASE + SPACC_CTX_HASH_KEY);

	/* init software valiable, crypto algo select */
	spacc_open(&sj, cipher, hash);

	/* write context to context buffer, that is write key, iv to io register */
	if (!hash)
		spacc_write_context(&sj, &ctx, SPACC_CRYPTO_OPERATION, key,
			SPACC_AES_KEY_SIZE, iv, SPACC_AES_IV_SIZE);
	else
		spacc_write_context(&sj, &ctx, SPACC_HASH_OPERATION, 0, 0, 0, 0);

	/* setup some operation like encrypt ctrl and icv option */
	spacc_set_operation(&sj, encrypt ? OP_ENCRYPT : OP_DECRYPT, 0, 0, 0, 0,
						0);
	if (!encrypt) {
		spacc_set_key_exp(&sj, &ctx);
	}

	/* process enqueue/dequeue works */
	if (!hash) {
		do {
			if ((src_len - curr_len) < MAX_PROC_LEN)
				chunk_len = src_len - curr_len;
			else
				chunk_len = MAX_PROC_LEN;

			spacc_packet_enqueue(&sj, hash, src, dst, chunk_len,
								 0, 0, 0, 0);
			if (spacc_packet_dequeue(JOB_IDX) != CRYPTO_OK) {
				printf("Error: spacc packet dequeue \n");
				return CRYPTO_FAILED;
			}

			src += chunk_len;
			dst += chunk_len;

			curr_len += chunk_len;
		} while (curr_len < src_len);
	} else {
		//make it align to hash_block_size
		unsigned int hash_set_blk_mod = 0;
		unsigned int hash_set_blk_num = 0;
		unsigned int i = 0;
		unsigned int hash_set_l2_sz, l2_sz_mod;
		hash_set_blk_num = src_len / MAX_PROC_LEN;
		hash_set_blk_mod = src_len % MAX_PROC_LEN;

		if (hash_set_blk_mod != 0)
			hash_set_blk_num += 1;

		for (i = 0; i < hash_set_blk_num; i++) {
			if ((src_len - curr_len) < MAX_PROC_LEN)
				chunk_len = src_len - curr_len;
			else
				chunk_len = MAX_PROC_LEN;

			debug("still in part src = %llx, dst = %llx len = %02x\n", src,
			dst + SHA256_SIZE * i, chunk_len);

			spacc_packet_enqueue(&sj, hash, src,
								 dst + SHA256_SIZE * i, chunk_len, 0,
								 chunk_len, 0, 0);
			if (spacc_packet_dequeue(JOB_IDX) != CRYPTO_OK)
				return CRYPTO_FAILED;

			src += chunk_len;
			curr_len += chunk_len;
		}

		if (src_len > MAX_PROC_LEN) {
			hash_set_l2_sz = hash_set_blk_num * SHA256_SIZE;
			l2_sz_mod = hash_set_l2_sz % HASH_SET_BLOCK_SIZE;

			if (l2_sz_mod != 0) {
				memset((void *)(dst + hash_set_l2_sz), 0,
					HASH_SET_BLOCK_SIZE - l2_sz_mod);
				hash_set_l2_sz =
					hash_set_l2_sz + HASH_SET_BLOCK_SIZE - l2_sz_mod;
			}
			spacc_packet_enqueue(&sj, hash, dst, dst,
								 hash_set_l2_sz, 0, hash_set_l2_sz, 0, 0);
			if (spacc_packet_dequeue(JOB_IDX) != CRYPTO_OK)
				return CRYPTO_FAILED;
		}
	}
	return CRYPTO_OK;
}

int spacc_init(void)
{
	/* determine max PROClen value */
	mmio_write_32(SPACC_BASE + SPACC_REG_PROC_LEN, 0xFFFFFFFF);

	/* Read spacc version */
	debug("SPACC Init - ID: (%02x)\n",
		(unsigned int) mmio_read_32(SPACC_BASE + SPACC_REG_ID));

	/* Clear iv offset */
	mmio_write_32(SPACC_BASE + SPACC_REG_IV_OFFSET, 0x80000000);
	if ((mmio_read_32(SPACC_BASE + SPACC_REG_IV_OFFSET)) != (0x80000000)) {
		printf("spacc_init::Failed to set iv offset\n");
		return -1;
	}

	mmio_read_32(SPACC_BASE + SPACC_REG_CONFIG);
	mmio_read_32(SPACC_BASE + SPACC_REG_CONFIG2);

	/* PROC LEN test */
	mmio_write_32(SPACC_BASE + SPACC_REG_PROC_LEN, 0xFFFFFFFF);
	if ((mmio_read_32(SPACC_BASE + SPACC_REG_PROC_LEN)) != (0x0000FFFF)) {
		printf("spacc_init::Failed to set proc len\n");
		return -1;
	}
	/* Clear src/dest value */
	mmio_write_32(SPACC_BASE + SPACC_REG_DST_PTR, 0x1234567F);
	mmio_write_32(SPACC_BASE + SPACC_REG_SRC_PTR, 0xDEADBEEF);

	if (((mmio_read_32(SPACC_BASE + SPACC_REG_DST_PTR)) !=
		 (0x1234567F & SPACC_DST_PTR_PTR))
		|| ((mmio_read_32(SPACC_BASE + SPACC_REG_SRC_PTR)) !=
			(0xDEADBEEF & SPACC_SRC_PTR_PTR))) {
		printf("spacc_init::Failed to set pointers\n");
		return -1;
	}
	/* zero the IRQ CTRL/EN register (to make sure we're in a sane state) */
	mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_CTRL, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_EN, 0x80000011);
	mmio_write_32(SPACC_BASE + SPACC_REG_IRQ_STAT, 0xFFFFFFFF);

	/* init regs */
	mmio_write_32(SPACC_BASE + SPACC_REG_SRC_PTR, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_DST_PTR, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_PROC_LEN, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_ICV_LEN, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_ICV_OFFSET, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_PRE_AAD_LEN, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_POST_AAD_LEN, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_IV_OFFSET, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_OFFSET, 0);
	mmio_write_32(SPACC_BASE + SPACC_REG_AUX_INFO, 0);

	return 0;
}


#if SPACC_TEST

#define SHA256_TEST_ADDR   0x4000fc00
#define SHA256_SIMPLE_SIZE 0x200

#if 0
#define SHA256_TEST_LONG_ADDR_SRC   0x20000000
#define SHA256_LONG_SIZE	 (128*1024)
#define SHA256_TEST_LONG_ADDR_DEST  (0x20000000 + SHA256_LONG_SIZE)
#else
#define SHA256_TEST_LONG_ADDR2_SRC   (0x4021fc00)
#define SHA256_TEST_LONG_ADDR2_DEST  (0x60000000)

#define SHA256_TEST_LONG_ADDR_SRC   (0x8021fc00)
#define SHA256_TEST_LONG_ADDR_DEST  (0x40000000)
#define SHA256_LONG_SIZE	 (0x680)
#endif
#define AES_128_TEST_ADDR_SRC   0x80000000
#define AES_128_TEST_ADDR_DEST  0x80000000
#define AES_TEST_SIMPLE_SIZE   0x200
#define AES_128_TEST_ADDR_LONG_SRC   0x20000000
#define AES_128_TEST_ADDR_LONG_DEST   0x20000000
#define AES_TEST_SIMPLE_LONG_SIZE   (20*1024*1024)

unsigned int hash_cmp[8] = {
	0x3f799818, 0xa81d116c, 0x4413da1d, 0x93733572,
	0xf556544c, 0x18692bd3, 0xc785781a, 0x500018f7
};

unsigned char msg[] =
	"I could stay awake just to hear you breathing Watch you smile while you"
	" are sleeping While you're far away dreaming I could spend my life in"
	" this sweet surrender I could stay lost in this moment forever Every"
	" moment spent with you is a moment I treasure Don't want to close my"
	" eyes I don't want to fall asleep 'Cause I'd miss you baby And I don't"
	" want to miss a thing 'Cause even when I dream of you The sweetest dream"
	" will never do I'd still miss you baby And I don't want to miss a thing"
	" And I don't want to mi";

const unsigned char aes_test_key[16] = "0123456789012345";
const unsigned char aes_test_iv[16] = "0123456789012345";

const unsigned char aes_test_key_long[16] = {
	0x01, 0x5c, 0x25, 0x0a, 0xe0, 0x03, 0x78, 0x5c,
	0x7b, 0x81, 0x00, 0xf0, 0x2b, 0xe5, 0x7f, 0xec
};

const unsigned char aes_test_iv_long[16] = {
	0x26, 0xa4, 0xd7, 0xdb, 0xf7, 0x1b, 0xd2, 0x6b,
	0x3c, 0xe9, 0x98, 0xb2, 0x4b, 0x2a, 0x22, 0x84
};

void spacc_sha256_simple(void)
{
	unsigned int i;

	memcpy((void *) SHA256_TEST_ADDR, msg, SHA256_SIMPLE_SIZE);
	flush_dcache_range(SHA256_TEST_ADDR, SHA256_SIMPLE_SIZE);
	spacc_ex(SHA256_TEST_ADDR, SHA256_TEST_ADDR, SHA256_SIMPLE_SIZE, 0, 1,
			 CRYPTO_MODE_HASH_SHA256, 0, 0, 0, 0);

	if (!memcmp((void *) hash_cmp, (void *) SHA256_TEST_ADDR, 32)) {
		printf("spacc_sha256_simple test pass!! \n");
	} else {
		printf("hash fail: \n");
		printf("hash compare data as:\n");
		for (i = 0; i < 8; i++) {
			printf("%x ", hash_cmp[i]);
			if (i % 4 == 3)
				printf("\n");
		}

		printf("hash result as:\n");
		for (i = 0; i < 8; i++) {
			printf("%x ", (unsigned int)
				(*((uintptr_t *)	(SHA256_TEST_ADDR + i * 4))));
			if (i % 4 == 3)
				printf("\n");
		}
	}
}

void spacc_sha256_long(void)
{
	unsigned int j = 0;

	spacc_ex(SHA256_TEST_LONG_ADDR_SRC, SHA256_TEST_LONG_ADDR_DEST,
			 SHA256_LONG_SIZE, 0, 1, CRYPTO_MODE_HASH_SHA256, 0, 0, 0, 0);

	mdelay(100);
	for (j = 0; j < 8; j++) {
		printf(" %x ", (unsigned int)
			(*((uintptr_t *) (SHA256_TEST_LONG_ADDR_DEST + j * 4))));
		if (j % 4 == 3)
			printf("\n");
	}

	spacc_ex(SHA256_TEST_LONG_ADDR2_SRC, SHA256_TEST_LONG_ADDR2_DEST,
			 SHA256_LONG_SIZE, 0, 1, CRYPTO_MODE_HASH_SHA256, 0, 0, 0, 0);
	mdelay(100);
	for (j = 0; j < 8; j++) {
		printf(" %x ", (unsigned int)
			(*((uintptr_t *) (SHA256_TEST_LONG_ADDR2_DEST + j * 4))));
		if (j % 4 == 3)
			printf("\n");
	}
}

void spacc_aes_simple(void)
{
	unsigned char *dec = (void *) AES_128_TEST_ADDR_DEST;
	int i = 0;

	spacc_ex((uint64_t) & msg, AES_128_TEST_ADDR_SRC,
			 AES_TEST_SIMPLE_SIZE, CRYPTO_MODE_AES_CBC, 1, 0, aes_test_key,
			 16, aes_test_iv, 16);

	spacc_ex(AES_128_TEST_ADDR_SRC, AES_128_TEST_ADDR_DEST,
			 AES_TEST_SIMPLE_SIZE, CRYPTO_MODE_AES_CBC, 0, 0, aes_test_key,
			 16, aes_test_iv, 16);

	invalidate_dcache_range((uintptr_t) AES_128_TEST_ADDR_DEST,
					 AES_TEST_SIMPLE_SIZE);

	if (!memcmp((void *) dec, (void *) AES_128_TEST_ADDR_DEST,
		 AES_TEST_SIMPLE_SIZE))
		printf("aes simple test pass!!! \n");
	else
		printf("aes simple test fail...\n");

#if 0
	for (i = 0; i < AES_TEST_SIMPLE_SIZE; i++) {
		printf("%c", *(dec+i));

		if (i % 64 == 63)
			printf("\n");
	}
	printf("\n");
#endif
}

void spacc_aes_long(void)
{
	spacc_ex(AES_128_TEST_ADDR_LONG_SRC, AES_128_TEST_ADDR_LONG_DEST,
			 AES_TEST_SIMPLE_LONG_SIZE, CRYPTO_MODE_AES_CBC, 0, 0,
			 aes_test_key_long, 16, aes_test_iv_long, 16);

	printf("aes long done\n");
}

void spacc_test(void)
{
	spacc_init();

	printf("============================================\n");
	spacc_aes_test();
	printf("============================================\n");
	spacc_hash_test();
	printf("=============================================\n");
}

#endif
