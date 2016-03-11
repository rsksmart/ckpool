/*
 * Copyright 2016
 */

#ifndef ROOTSTOCK_H
#define ROOTSTOCK_H

#include "config.h"

struct rootstock_data {
	ckpool_t *ckp;

	/* Current server instance */
	server_instance_t *si;

	/* Id for last request */
	int lastreqid;

	/* Time we of last getwork */
	time_t last_getwork;

	/* Last getWork parameters */
	char blockhashmerge[32];
	double difficulty;
};

typedef struct rootstock_data rdata_t;

struct rsk_getwork {
	char blockhashmerge[32];
	double difficulty;
};

typedef struct rsk_getwork rsk_getwork_t;

void *rootstock(void *arg);

#endif /* ROOTSTOCK_H */
