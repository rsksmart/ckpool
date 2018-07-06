/*
 * Copyright 2018
 */

#ifndef EMERCOIN_H
#define EMERCOIN_H

#include "config.h"

#define BIN_HASH_SIZE 32
#define STR_HEX_HASH_SIZE 65

struct emercoin_data {
    ckpool_t *ckp;

	/* Current server instance */
	server_instance_t *si;

	/* Id for last request */
	int lastreqid;

	/* Last getauxblock parameters */
	char hashmergebin[BIN_HASH_SIZE];
	char hashmerge[STR_HEX_HASH_SIZE];
	char target[STR_HEX_HASH_SIZE];
	int chainid;

	/* Last hash being worked on */
	char lasthashmerge[STR_HEX_HASH_SIZE];
};

typedef struct emercoin_data emcdata_t;

struct emc_auxblock {
	char hashmergebin[BIN_HASH_SIZE];
	char target[STR_HEX_HASH_SIZE];
	int chainid;

	char hashmerge[STR_HEX_HASH_SIZE];
};

typedef struct emc_auxblock emc_auxblock_t;

void* emercoin(void* arg);

#endif /* EMERCOIN_H */

