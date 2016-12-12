/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under 
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*****************************************************************************
Source      esmData.h

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines internal private data handled by EPS Session
        Management sublayer.

*****************************************************************************/

#ifndef __ESMDATA_H__
#define __ESMDATA_H__

#include "nas_timer.h"
#include "networkDef.h"
#include "tree.h"
#include "3gpp_24.007.h"
#include "mme_api.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Total number of active EPS bearers */
#define ESM_DATA_EPS_BEARER_TOTAL   11

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Minimal and maximal value of an EPS bearer identity:
 * The EPS Bearer Identity (EBI) identifies a message flow
 */
#define ESM_EBI_MIN     (EPS_BEARER_IDENTITY_FIRST)
#define ESM_EBI_MAX     (EPS_BEARER_IDENTITY_LAST)

/* EPS bearer context states */
typedef enum {
  ESM_EBR_INACTIVE,       /* No EPS bearer context exists     */
  ESM_EBR_ACTIVE,     /* The EPS bearer context is active,
                 * in the UE, in the network        */
  ESM_EBR_INACTIVE_PENDING,   /* The network has initiated an EPS bearer
                 * context deactivation towards the UE  */
  ESM_EBR_MODIFY_PENDING, /* The network has initiated an EPS bearer
                 * context modification towards the UE  */
  ESM_EBR_ACTIVE_PENDING, /* The network has initiated an EPS bearer
                 * context activation towards the UE    */
  ESM_EBR_STATE_MAX
} esm_ebr_state;

/* ESM message timer retransmission data */
typedef struct esm_ebr_timer_data_s {
  void            *ctx;
  mme_ue_s1ap_id_t ue_id;      /* Lower layers UE identifier       */
  ebi_t            ebi;       /* EPS bearer identity          */
  unsigned int     count;     /* Retransmission counter       */
  bstring          msg;        /* Encoded ESM message to re-transmit   */
} esm_ebr_timer_data_t;

/*
 * -----------------------
 * EPS bearer context data
 * -----------------------
 */
typedef struct esm_ebr_context_s {
  //ebi_t ebi;      /* EPS bearer identity          */
  esm_ebr_state status;   /* EPS bearer context status        */

  struct nas_timer_s timer;   /* Retransmission timer         */
  esm_ebr_timer_data_t *args; /* Retransmission timer parameters data */
} esm_ebr_context_t;

typedef struct esm_ebr_data_s {
  unsigned char index;    /* Index of the next EPS bearer context
                 * identity to be used */
#define ESM_EBR_DATA_SIZE (ESM_EBI_MAX - ESM_EBI_MIN + 1)
  esm_ebr_context_t *context[ESM_EBR_DATA_SIZE + 1];
} esm_ebr_data_t;

/*
 * --------------------------------------------------------------------------
 * Structure of data handled by EPS Session Management sublayer in the UE
 * and in the MME
 * --------------------------------------------------------------------------
 */

/*
 * Structure of an EPS bearer
 * --------------------------
 * An EPS bearer is a logical concept which applies to the connection
 * between two endpoints (UE and PDN Gateway) with specific QoS attri-
 * butes. An EPS bearer corresponds to one Quality of Service policy
 * applied within the EPC and E-UTRAN.
 */
typedef struct esm_bearer_s {
  int bid;        /* Identifier of the EPS bearer         */
  unsigned int ebi;   /* EPS bearer identity              */
  network_qos_t qos;  /* EPS bearer level QoS parameters      */
  traffic_flow_template_t *tft;  /* Traffic Flow Template for packet filtering   */
} esm_bearer_t;

/*
 * Structure of a PDN connection
 * -----------------------------
 * A PDN connection is the association between a UE represented by
 * one IPv4 address and/or one IPv6 prefix and a PDN represented by
 * an Access Point Name (APN).
 */
typedef struct esm_pdn_s {
  proc_tid_t  pti;   /* Identity of the procedure transaction executed
             * to activate the PDN connection entry     */
  bool is_emergency;   /* Emergency bearer services indicator      */
  int ambr;       /* Aggregate Maximum Bit Rate of this APN   */

  int addr_realloc;   /* Indicates whether the UE is allowed to subsequently
             * request another PDN connectivity to the same APN
             * using an address PDN type (IPv4 or IPv6) other
             * than the one already activated       */
  int n_bearers;  /* Number of allocated EPS bearers;
             * default EPS bearer is defined at index 0 */
} esm_pdn_t;

struct esm_proc_data_s;

/*
 * Structure of the ESM data
 * -------------------------
 * The EPS Session Management sublayer handles data related to PDN
 * connections and EPS bearers. Each active PDN connection has a de-
 * fault EPS bearer. Several dedicated EPS bearers may exist within
 * a PDN connection.
 */
typedef struct esm_context_s {
  //mme_ue_s1ap_id_t ue_id;

  int n_active_ebrs;     /* Total number of active EPS bearer contexts   */
  int n_active_pdns;     /* Number of active PDN connections     */
  int n_pdns;
  bool is_emergency;  /* Indicates whether a PDN connection for emergency
             * bearer services is established       */
  struct esm_proc_data_s *esm_proc_data;
} esm_context_t;



/*
 * --------------------------------------------------------------------------
 *  ESM internal data handled by EPS Session Management sublayer in the MME
 * --------------------------------------------------------------------------
 */
/*
 * Structure of the ESM data
 * -------------------------
 */
struct mme_api_esm_config_s;

typedef struct esm_data_s {
  /*
   * MME configuration
   * -----------------
   */
  struct mme_api_esm_config_s conf;

} esm_data_t;


void free_esm_data_context(esm_context_t * esm_data_ctx);
void esm_init_context(struct esm_context_s *esm_ctx);


struct esm_context_s *esm_data_context_get(
  esm_data_t *esm_data, unsigned int _ueid);

struct esm_context_s *esm_data_context_remove(
  esm_data_t *esm_data, struct esm_context_s *elm);

void esm_data_context_add(esm_data_t *esm_data, struct esm_context_s *elm);


/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/*
 * ESM internal data (used within ESM only)
 * ----------------------------------------
 */
esm_data_t _esm_data;

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

extern char ip_addr_str[100];

extern char *esm_data_get_ipv4_addr(const_bstring ip_addr);

extern char *esm_data_get_ipv6_addr(const_bstring ip_addr);

extern char *esm_data_get_ipv4v6_addr(const_bstring ip_addr);

#endif /* __ESMDATA_H__*/
