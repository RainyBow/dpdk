/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018 Intel Corporation
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/**
 * @file crypto.h
 * Contains crypto specific functions/structures/macros used internally
 * by ipsec library.
 */

 /*
  * AES-GCM devices have some specific requirements for IV and AAD formats.
  * Ideally that to be done by the driver itself.
  */

struct aead_gcm_iv {
	uint32_t salt;
	uint64_t iv;
	uint32_t cnt;
} __attribute__((packed));

struct aead_gcm_aad {
	uint32_t spi;
	/*
	 * RFC 4106, section 5:
	 * Two formats of the AAD are defined:
	 * one for 32-bit sequence numbers, and one for 64-bit ESN.
	 */
	union {
		uint32_t u32[2];
		uint64_t u64;
	} sqn;
	uint32_t align0; /* align to 16B boundary */
} __attribute__((packed));

struct gcm_esph_iv {
	struct esp_hdr esph;
	uint64_t iv;
} __attribute__((packed));


static inline void
aead_gcm_iv_fill(struct aead_gcm_iv *gcm, uint64_t iv, uint32_t salt)
{
	gcm->salt = salt;
	gcm->iv = iv;
	gcm->cnt = rte_cpu_to_be_32(1);
}

/*
 * RFC 4106, 5 AAD Construction
 * spi and sqn should already be converted into network byte order.
 * Make sure that not used bytes are zeroed.
 */
static inline void
aead_gcm_aad_fill(struct aead_gcm_aad *aad, rte_be32_t spi, rte_be64_t sqn,
	int esn)
{
	aad->spi = spi;
	if (esn)
		aad->sqn.u64 = sqn;
	else {
		aad->sqn.u32[0] = sqn_low32(sqn);
		aad->sqn.u32[1] = 0;
	}
	aad->align0 = 0;
}

static inline void
gen_iv(uint64_t iv[IPSEC_MAX_IV_QWORD], rte_be64_t sqn)
{
	iv[0] = sqn;
	iv[1] = 0;
}

/*
 * from RFC 4303 3.3.2.1.4:
 * If the ESN option is enabled for the SA, the high-order 32
 * bits of the sequence number are appended after the Next Header field
 * for purposes of this computation, but are not transmitted.
 */

/*
 * Helper function that moves ICV by 4B below, and inserts SQN.hibits.
 * icv parameter points to the new start of ICV.
 */
static inline void
insert_sqh(uint32_t sqh, void *picv, uint32_t icv_len)
{
	uint32_t *icv;
	int32_t i;

	RTE_ASSERT(icv_len % sizeof(uint32_t) == 0);

	icv = picv;
	icv_len = icv_len / sizeof(uint32_t);
	for (i = icv_len; i-- != 0; icv[i] = icv[i - 1])
		;

	icv[i] = sqh;
}

/*
 * Helper function that moves ICV by 4B up, and removes SQN.hibits.
 * icv parameter points to the new start of ICV.
 */
static inline void
remove_sqh(void *picv, uint32_t icv_len)
{
	uint32_t i, *icv;

	RTE_ASSERT(icv_len % sizeof(uint32_t) == 0);

	icv = picv;
	icv_len = icv_len / sizeof(uint32_t);
	for (i = 0; i != icv_len; i++)
		icv[i] = icv[i + 1];
}

#endif /* _CRYPTO_H_ */