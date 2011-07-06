/*
 * sm-iasecc.c: Secure Messaging procedures specific to IAS/ECC card
 *
 * Copyright (C) 2010  Viktor Tarasov <vtarasov@opentrust.com>
 *					  OpenTrust <www.opentrust.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include <openssl/des.h>

#include "libopensc/opensc.h"
#include "libopensc/sm.h"
#include "libopensc/log.h"
#include "libopensc/asn1.h"
#include "libopensc/iasecc.h"
#include "libopensc/iasecc-sdo.h"
#if 0
#include "libopensc/hash-strings.h"
#endif
#include "sm-module.h"

static const struct sc_asn1_entry c_asn1_card_response[2] = {
	{ "cardResponse", SC_ASN1_STRUCT, SC_ASN1_CTX | 1 | SC_ASN1_CONS, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};
static const struct sc_asn1_entry c_asn1_iasecc_response[4] = {
	{ "number",	SC_ASN1_INTEGER,	SC_ASN1_TAG_INTEGER,    0, NULL, NULL },
	{ "status",	SC_ASN1_INTEGER, 	SC_ASN1_TAG_INTEGER,    0, NULL, NULL },
	{ "data",       SC_ASN1_OCTET_STRING,   SC_ASN1_CTX | 2 | SC_ASN1_CONS, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};
static const struct sc_asn1_entry c_asn1_iasecc_sm_response[4] = {
	{ "number",	SC_ASN1_INTEGER,	SC_ASN1_TAG_INTEGER,    0, NULL, NULL },
	{ "status",	SC_ASN1_INTEGER, 	SC_ASN1_TAG_INTEGER,    0, NULL, NULL },
	{ "data",	SC_ASN1_STRUCT,		SC_ASN1_CTX | 2 | SC_ASN1_CONS, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};
static const struct sc_asn1_entry c_asn1_iasecc_sm_data_object[4] = {
	{ "encryptedData", 	SC_ASN1_OCTET_STRING,	SC_ASN1_CTX | 7,	SC_ASN1_OPTIONAL,	NULL, NULL },
	{ "commandStatus", 	SC_ASN1_OCTET_STRING,	SC_ASN1_CTX | 0x19,	0, 			NULL, NULL },
	{ "ticket", 		SC_ASN1_OCTET_STRING,	SC_ASN1_CTX | 0x0E,	0, 			NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};


#if 0
static int
sm_iasecc_get_apdu_read_binary(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_read_binary *rb = &sm_info->cmd_params.read_binary;
	size_t offs = rb->offset, size = rb->size;
	int rv = SC_ERROR_INVALID_ARGUMENTS;

	LOG_FUNC_CALLED(ctx);
	while (size)   {
		int sz = size > SM_MAX_DATA_SIZE ? SM_MAX_DATA_SIZE : size;
		struct sc_remote_apdu *rapdu = NULL;

		sc_log(ctx, "SM get 'READ BINARY' APDUs: offset:%i,size:%i", offs, size);
		rv = sc_remote_apdu_allocate(rapdus, &rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'READ BINARY' APDUs: cannot allocate remote apdu");

		rapdu->apdu.cse = SC_APDU_CASE_2_SHORT;
		rapdu->apdu.cla = 0x00;
		rapdu->apdu.ins = 0xB0;
		rapdu->apdu.p1 = (offs>>8)&0xFF;
		rapdu->apdu.p2 = offs&0xFF;
		rapdu->apdu.resplen = sz;
		rapdu->apdu.le = sz;

		rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'READ BINARY' APDUs: securize error");

		rapdu->get_response = 1;

		offs += sz;
		size -= sz;
	}
			
	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_update_binary(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_update_binary *ub = &sm_info->cmd_params.update_binary;
	size_t offs = ub->offset, size = ub->size, data_offs = 0;
	int rv = SC_ERROR_INVALID_ARGUMENTS;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'UPDATE BINARY' APDUs: offset:%i,size:%i", offs, size);
	while (size)   {
		int sz = size > SM_MAX_DATA_SIZE ? SM_MAX_DATA_SIZE : size;
		struct sc_remote_apdu *rapdu = NULL;

		rv = sc_remote_apdu_allocate(rapdus, &rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'UPDATE BINARY' APDUs: cannot allocate remote apdu");

		rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
		rapdu->apdu.cla = 0x00;
		rapdu->apdu.ins = 0xD6;
		rapdu->apdu.p1 = (offs>>8)&0xFF;
		rapdu->apdu.p2 = offs&0xFF;
		memcpy((unsigned char *)rapdu->apdu.data, ub->data + data_offs, sz);
		rapdu->apdu.datalen = sz;
		rapdu->apdu.lc = sz;

		rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'UPDATE BINARY' APDUs: securize error");

		rapdu->get_response = 1;

		offs += sz;
		data_offs += sz;
		size -= sz;
	}
			
	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_create_file(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_create_file *cf = &sm_info->cmd_params.create_file;
	struct sc_remote_apdu *rapdu = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'CREATE FILE' APDU: FCP(%i) %p", cf->fcp_len, cf->fcp);

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'CREATE FILE' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0xE0;
	rapdu->apdu.p1 = 0x00;
	rapdu->apdu.p2 = 0x00;
	memcpy((unsigned char *)rapdu->apdu.data, cf->fcp, cf->fcp_len);
	rapdu->apdu.datalen = cf->fcp_len;
	rapdu->apdu.lc = cf->fcp_len;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'CREATE FILE' APDU: securize error");

	rapdu->get_response = 1;

	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_delete_file(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_delete_file *df = &sm_info->cmd_params.delete_file;
	struct sc_remote_apdu *rapdu = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'DELETE FILE' APDU: file-id %04X", df->file_id);

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'DELETE FILE' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_1;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0xE4;
	rapdu->apdu.p1 = 0x00;
	rapdu->apdu.p2 = 0x00;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'DELETE FILE' APDU: securize error");

	rapdu->get_response = 1;

	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_verify_pin(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_pin_verify *pv = &sm_info->cmd_params.pin_verify;
	struct sc_remote_apdu *rapdu = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'VERIFY PIN' APDU");

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'VERIFY PIN' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0x20;
	rapdu->apdu.p1 = 0x00;
	rapdu->apdu.p2 = pv->pin.reference;
	if (pv->pin.size > SM_MAX_DATA_SIZE)
		LOG_TEST_RET(ctx, rv, "SM get 'VERIFY PIN' APDU: invelid PIN size");

	memcpy((unsigned char *)rapdu->apdu.data, pv->pin.data, pv->pin.size);
	rapdu->apdu.datalen = pv->pin.size;
	rapdu->apdu.lc = pv->pin.size;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'VERIFY_PIN' APDU: securize error");

	rapdu->get_response = 1;

	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_reset_pin(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_pin_reset *pr = &sm_info->cmd_params.pin_reset;
	struct sc_remote_apdu *rapdu = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'RESET PIN' APDU");

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'RESET PIN' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0x2C;
	rapdu->apdu.p2 = pr->pin2.reference;
	if (pr->pin2.size)   {
		if (pr->pin2.size > SM_MAX_DATA_SIZE)
			LOG_TEST_RET(ctx, rv, "SM get 'RESET PIN' APDU: invelid PIN size");

		rapdu->apdu.p1 = 0x02;
		memcpy((unsigned char *)rapdu->apdu.data, pr->pin2.data, pr->pin2.size);
		rapdu->apdu.datalen = pr->pin2.size;
		rapdu->apdu.lc = pr->pin2.size;
	}
	else   {
		rapdu->apdu.p1 = 0x03;
	}

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'RESET_PIN' APDU: securize error");

	rapdu->get_response = 1;

	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_generate_rsa(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_rsa_generate *rg = &sm_info->cmd_params.rsa_generate;
	struct sc_remote_apdu *rapdu = NULL;
	unsigned char put_exponent_data[14] = { 
		0x70, 0x0C, 
			IASECC_SDO_TAG_HEADER, IASECC_SDO_CLASS_RSA_PUBLIC | 0x80, rg->reference & 0x7F, 0x08, 
					0x7F, 0x49, 0x05, 0x82, 0x03, 0x01, 0x00, 0x01 
	};
	unsigned char generate_data[5] = { 
		0x70, 0x03, 
			IASECC_SDO_TAG_HEADER, IASECC_SDO_CLASS_RSA_PRIVATE | 0x80, rg->reference & 0x7F
	};
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'GENERATE RSA' APDU: SDO(class:%X,reference:%X)", rg->sdo_class, rg->reference);

	/* Put Exponent */
	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'GENERATE RSA(put exponent)' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0xDB;
	rapdu->apdu.p1 = 0x3F;
	rapdu->apdu.p2 = 0xFF;
	memcpy((unsigned char *)rapdu->apdu.data, put_exponent_data, sizeof(put_exponent_data));
	rapdu->apdu.datalen = sizeof(put_exponent_data);
	rapdu->apdu.lc = sizeof(put_exponent_data);

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'GENERATE RSA(put exponent)' APDU: securize error");

	rapdu->get_response = 1;

	/* Generate Key */
	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'GENERATE RSA' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_4_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0x47;
	rapdu->apdu.p1 = 0x00;
	rapdu->apdu.p2 = 0x00;
	memcpy((unsigned char *)rapdu->apdu.data, generate_data, sizeof(generate_data));
	rapdu->apdu.datalen = sizeof(generate_data);
	rapdu->apdu.lc = sizeof(generate_data);
	rapdu->apdu.le = 0x100;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'GENERATE RSA' APDU: securize error");

	rapdu->get_response = 1;

	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_update_rsa(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_iasecc_rsa_update *iru = &sm_info->cmd_params.iasecc_rsa_update;
	struct sc_iasecc_sdo_rsa_update *ru = iru->data;
	struct sc_iasecc_sdo_update *to_update[2];
	struct sc_remote_apdu *rapdu = NULL;
	int rv, ii, jj;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'UPDATE RSA' APDU: SDO(class:%X,reference:%X)", iru->sdo_class, iru->reference);
	if (ru->magic != IASECC_SDO_MAGIC_UPDATE_RSA)
		LOG_TEST_RET(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED, "SM get 'UPDATE RSA' APDU: invalid magic");

	to_update[0] = &ru->update_prv;
	to_update[1] = &ru->update_pub;
	for (jj=0;jj<2;jj++)   {
		for (ii=0; to_update[jj]->fields[ii].tag && ii < IASECC_SDO_TAGS_UPDATE_MAX; ii++)   {
			unsigned char *encoded = NULL;
			size_t encoded_len, offs;

			sc_log(ctx, "SM get 'UPDATE RSA' APDU: comp %i:%i, SDO(class:%02X%02X)", jj, ii, 
					iru->sdo_class, iru->reference);
			encoded_len = iasecc_sdo_encode_update_field(ctx, to_update[jj]->sdo_class, to_update[jj]->sdo_ref,
						&to_update[jj]->fields[ii], &encoded);
			LOG_TEST_RET(ctx, encoded_len, "SM get 'UPDATE RSA' APDU: cannot encode key component");
			
			sc_log(ctx, "SM IAS/ECC get APDUs: component(num:%i:%i,class:%X,ref:%X,%s)", jj, ii,
					to_update[jj]->sdo_class, to_update[jj]->sdo_ref,
					sc_dump_hex(encoded, encoded_len));

			for (offs = 0; offs < encoded_len; )   {
				int len = encoded_len - offs > SM_MAX_DATA_SIZE ? SM_MAX_DATA_SIZE : encoded_len - offs;

				rv = sc_remote_apdu_allocate(rapdus, &rapdu);
				LOG_TEST_RET(ctx, rv, "SM get 'UPDATE RSA' APDU: cannot allocate remote apdu");

				rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
				rapdu->apdu.cla = len + offs < encoded_len ? 0x10 : 0x00;
				rapdu->apdu.ins = 0xDB;
				rapdu->apdu.p1 = 0x3F;
				rapdu->apdu.p2 = 0xFF;
				memcpy((unsigned char *)rapdu->apdu.data, encoded + offs, len);
				rapdu->apdu.datalen = len;
				rapdu->apdu.lc = len;

				rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
				LOG_TEST_RET(ctx, rv, "SM get 'UPDATE RSA' APDU: securize error");

				rapdu->get_response = 1;

				offs += len;
			}
			free(encoded);
		}
	}

	LOG_FUNC_RETURN(ctx, SC_SUCCESS);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_pso_dst(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sm_info_iasecc_pso_dst *ipd = &sm_info->cmd_params.iasecc_pso_dst;
	struct sc_remote_apdu *rapdu = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'PSO DST' APDU");

	if (ipd->pso_data_len > SM_MAX_DATA_SIZE)
		LOG_FUNC_RETURN(ctx, SC_ERROR_INVALID_ARGUMENTS);

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'PSO HASH' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_3_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0x2A;
	rapdu->apdu.p1 = 0x90;
	rapdu->apdu.p2 = 0xA0;
	memcpy((unsigned char *)rapdu->apdu.data, ipd->pso_data, ipd->pso_data_len);
	rapdu->apdu.datalen = ipd->pso_data_len;
	rapdu->apdu.lc = ipd->pso_data_len;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'PSO HASH' APDU: securize error");

	rapdu->get_response = 1;

	rv = sc_remote_apdu_allocate(rapdus, &rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'PSO DST' APDU: cannot allocate remote apdu");

	rapdu->apdu.cse = SC_APDU_CASE_2_SHORT;
	rapdu->apdu.cla = 0x00;
	rapdu->apdu.ins = 0x2A;
	rapdu->apdu.p1 = 0x9E;
	rapdu->apdu.p2 = 0x9A;
	rapdu->apdu.le = ipd->key_size;

	rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
	LOG_TEST_RET(ctx, rv, "SM get 'PSO DST' APDU: securize error");

	rapdu->get_response = 1;


	LOG_FUNC_RETURN(ctx, rv);
}
#endif


#if 0
static int
sm_iasecc_get_apdu_raw_apdu(struct sc_context *ctx, struct sm_info *sm_info, struct sc_remote_apdu **rapdus)
{
	struct sc_apdu *apdu = sm_info->cmd_params.raw_apdu.apdu;
	size_t data_offs, data_len = apdu->datalen;
	int rv = SC_ERROR_INVALID_ARGUMENTS;

	LOG_FUNC_CALLED(ctx);
	sc_log(ctx, "SM get 'RAW APDU' APDU");

	data_offs = 0;
	data_len = apdu->datalen;
	for (; data_len; )   {
		int sz = data_len > SM_MAX_DATA_SIZE ? SM_MAX_DATA_SIZE : data_len;
		struct sc_remote_apdu *rapdu = NULL;

		rv = sc_remote_apdu_allocate(rapdus, &rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'RAW APDU' APDUs: cannot allocate remote apdu");

		rapdu->apdu.cse = apdu->cse;
		rapdu->apdu.cla = apdu->cla | ((data_offs + sz) < data_len ? 0x10 : 0x00);
		rapdu->apdu.ins = apdu->ins;
		rapdu->apdu.p1 = apdu->p1;
		rapdu->apdu.p2 = apdu->p2;
		memcpy((unsigned char *)rapdu->apdu.data, apdu->data + data_offs, sz);
		rapdu->apdu.datalen = sz;
		rapdu->apdu.lc = sz;

		rv = sm_iasecc_securize_apdu(ctx, sm_info, rapdu);
		LOG_TEST_RET(ctx, rv, "SM get 'UPDATE BINARY' APDUs: securize error");

		rapdu->get_response = 1;

		data_offs += sz;
		data_len -= sz;
	}
			
	LOG_FUNC_RETURN(ctx, rv);
}
#endif

int
sm_iasecc_get_apdus(struct sc_context *ctx, struct sm_info *sm_info, 
	       unsigned char *init_data, size_t init_len, struct sc_remote_data *rdata, int release_sm)
{
	struct sm_cwa_session *session_data = &sm_info->schannel.session.cwa;
	struct sm_cwa_keyset *keyset = &sm_info->schannel.keyset.cwa;
	int rv;

	LOG_FUNC_CALLED(ctx);
	if (!sm_info)
		LOG_FUNC_RETURN(ctx, SC_ERROR_INVALID_ARGUMENTS);

	sc_log(ctx, "SM IAS/ECC get APDUs: init_len:%i", init_len);
	sc_log(ctx, "SM IAS/ECC get APDUs: rdata:%p", rdata);
	sc_log(ctx, "SM IAS/ECC get APDUs: serial %s", sc_dump_hex(sm_info->serialnr.value, sm_info->serialnr.len));

	rv = sm_cwa_decode_authentication_data(ctx, keyset, session_data, init_data);
	LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: decode authentication data error");

	rv = sm_cwa_init_session_keys(ctx, session_data, sm_info->sm_params.cwa.crt_at.algo);
	LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: cannot get session keys");

	sc_log(ctx, "SKENC %s", sc_dump_hex(session_data->session_enc, sizeof(session_data->session_enc)));
	sc_log(ctx, "SKMAC %s", sc_dump_hex(session_data->session_mac, sizeof(session_data->session_mac)));
	sc_log(ctx, "SSC   %s", sc_dump_hex(session_data->ssc, sizeof(session_data->ssc)));

	switch (sm_info->cmd)  {
#if 0
	case SM_CMD_FILE_READ:
		rv = sm_iasecc_get_apdu_read_binary(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'READ BINARY' failed");
		break;
	case SM_CMD_FILE_UPDATE:
		rv = sm_iasecc_get_apdu_update_binary(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'UPDATE BINARY' failed");
		break;
	case SM_CMD_FILE_CREATE:
		rv = sm_iasecc_get_apdu_create_file(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'CREATE FILE' failed");
		break;
	case SM_CMD_FILE_DELETE:
		rv = sm_iasecc_get_apdu_delete_file(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'CREATE FILE' failed");
		break;
	case SM_CMD_PIN_RESET:
		rv = sm_iasecc_get_apdu_reset_pin(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'RESET PIN' failed");
		break;
	case SM_CMD_RSA_GENERATE:
		rv = sm_iasecc_get_apdu_generate_rsa(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'GENERATE RSA' failed");
		break;
	case SM_CMD_RSA_UPDATE:
		rv = sm_iasecc_get_apdu_update_rsa(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'UPDATE RSA' failed");
		break;
	case SM_CMD_PSO_DST:
		rv = sm_iasecc_get_apdu_pso_dst(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'PSO DST' failed");
		break;
	case SM_CMD_APDU_RAW:
		rv = sm_iasecc_get_apdu_raw_apdu(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'RAW APDU' failed");
		break;
	case SM_CMD_PIN_VERIFY:
		rv = sm_iasecc_get_apdu_verify_pin(ctx, sm_info, &rapdus);
		LOG_TEST_RET(ctx, rv, "SM IAS/ECC get APDUs: 'RAW APDU' failed");
		break;
#endif
	default:
		LOG_TEST_RET(ctx, SC_ERROR_NOT_SUPPORTED, "unsupported SM command");
	}

	if (release_sm)   {
		/* Apparently useless for this card */
	}

	LOG_FUNC_RETURN(ctx, rv);
}


int 
sm_iasecc_decode_card_data(struct sc_context *ctx, struct sm_info *sm_info, char *str_data, 
		unsigned char *out, size_t out_len)
{
#if 0
	struct sm_cwa_session *session_data = &sm_info->schannel.session.cwa;
	struct sc_asn1_entry asn1_iasecc_sm_data_object[4], asn1_iasecc_sm_response[4], asn1_card_response[2];
	struct sc_hash *hash = NULL;
	unsigned char *hex = NULL;
	size_t hex_len, len_left;
	int rv, offs;

	LOG_FUNC_CALLED(ctx);

	if (!out || !out_len)
		LOG_FUNC_RETURN(ctx, 0);

	sc_log(ctx, "IAS/ECC decode answer(s): out length %i", out_len);
	if (strstr(str_data, "DATA="))   {
		rv = sc_hash_parse(ctx, str_data, strlen(str_data), &hash);
		LOG_TEST_RET(ctx, rv, "IAS/ECC decode answer(s): parse error");

		str_data = sc_hash_get(hash, "DATA");
	}

	if (!strlen(str_data))
		LOG_FUNC_RETURN(ctx, 0);

	hex_len = strlen(str_data) / 2;
	hex = calloc(1, hex_len);
	if (!hex)
		LOG_TEST_RET(ctx, SC_ERROR_OUT_OF_MEMORY, "IAS/ECC decode answer(s): hex allocate error");

	rv = sc_hex_to_bin(str_data, hex, &hex_len);
	LOG_TEST_RET(ctx, rv, "IAS/ECC decode answer(s): data 'HEX to BIN' conversion error");
	sc_log(ctx, "IAS/ECC decode answer(s): hex length %i", hex_len);

	if (hash)
		sc_hash_free(hash);
	
	for (offs = 0, len_left = hex_len; len_left; )   {
                unsigned char *decrypted;
                size_t decrypted_len;
		unsigned char card_data[SC_MAX_APDU_BUFFER_SIZE], command_status[2];
		size_t card_data_len = sizeof(card_data), command_status_len = sizeof(command_status); 
		unsigned char ticket[8];
		size_t ticket_len = sizeof(ticket); 
		int num, status;

		sc_copy_asn1_entry(c_asn1_iasecc_sm_data_object, asn1_iasecc_sm_data_object);
		sc_copy_asn1_entry(c_asn1_iasecc_sm_response, asn1_iasecc_sm_response);
		sc_copy_asn1_entry(c_asn1_card_response, asn1_card_response);

		sc_format_asn1_entry(asn1_iasecc_sm_data_object + 0, card_data, &card_data_len, 0);
		sc_format_asn1_entry(asn1_iasecc_sm_data_object + 1, command_status, &command_status_len, 0);
		sc_format_asn1_entry(asn1_iasecc_sm_data_object + 2, ticket, &ticket_len, 0);

		sc_format_asn1_entry(asn1_iasecc_sm_response + 0, &num, NULL, 0);
		sc_format_asn1_entry(asn1_iasecc_sm_response + 1, &status, NULL, 0);
		sc_format_asn1_entry(asn1_iasecc_sm_response + 2, asn1_iasecc_sm_data_object, NULL, 0);

		sc_format_asn1_entry(asn1_card_response + 0, asn1_iasecc_sm_response, NULL, 0);

        	rv = sc_asn1_decode(ctx, asn1_card_response, hex + hex_len - len_left, len_left, NULL, &len_left);
		LOG_TEST_RET(ctx, rv, "IAS/ECC decode answer(s): ASN1 decode error");

		if (status != 0x9000)
			continue;

		if (asn1_iasecc_sm_data_object[0].flags & SC_ASN1_PRESENT)   {
			if (*card_data != 0x01)
				LOG_TEST_RET(ctx, SC_ERROR_INVALID_DATA, "IAS/ECC decode answer(s): invalid encrypted data format");

			decrypted_len = sizeof(decrypted);	
			rv = sm_decrypt_des_cbc3(ctx, session_data->session_enc, card_data + 1, card_data_len - 1, 
					&decrypted, &decrypted_len);
			LOG_TEST_RET(ctx, rv, "IAS/ECC decode answer(s): cannot decrypt card answer data");

			while(*(decrypted + decrypted_len - 1) == 0x00)
			       decrypted_len--;

			if (*(decrypted + decrypted_len - 1) != 0x80)
				LOG_TEST_RET(ctx, SC_ERROR_INVALID_DATA, "IAS/ECC decode answer(s): invalid card data padding ");

			decrypted_len--;

			if (out_len < offs + decrypted_len)
				LOG_TEST_RET(ctx, SC_ERROR_BUFFER_TOO_SMALL, "IAS/ECC decode answer(s): unsufficient output buffer size");

			memcpy(out + offs, decrypted, decrypted_len);
			offs += decrypted_len;
			sc_log(ctx, "IAS/ECC decode card answer(s): decrypted_len:%i, offs:%i", decrypted_len, offs);

			free(decrypted);
		}

		sc_log(ctx, "IAS/ECC decode card answer(s): decode answer: length left %i", len_left);
	}

	free(hex);
	LOG_FUNC_RETURN(ctx, offs);
#else
	LOG_FUNC_RETURN(ctx, SC_ERROR_NOT_SUPPORTED);
#endif
}
