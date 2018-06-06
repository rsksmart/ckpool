/*
 * Copyright 2018
 */

#ifndef EMERCOIN_H
#define EMERCOIN_H

#include "config.h"

struct emercoin_data {
    ckpool_t *ckp;

	/* Current server instance */
	server_instance_t *si;

	/* Id for last request */
	int lastreqid;

	/* Last getauxblock parameters */
	char hashmergebin[32];
	char hashmerge[65];
	char target[65];
	int chainid;

	/* Last hash being worked on */
	char lasthashmerge[65];
};

typedef struct emercoin_data emcdata_t;

struct emc_auxblock {
	char hashmergebin[32];
	char target[65];
	int chainid;

	char hashmerge[65];
};

typedef struct emc_auxblock emc_auxblock_t;

void* emercoin(void* arg);

#endif /* EMERCOIN_H */

