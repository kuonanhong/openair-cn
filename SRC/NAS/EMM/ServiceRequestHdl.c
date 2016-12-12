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
  Source      ServiceRequest.c

  Version     0.1

  Date        2013/05/07

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel

  Description Defines the service request EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the service request procedure is to transfer
        the EMM mode from EMM-IDLE to EMM-CONNECTED mode and establish
        the radio and S1 bearers when uplink user data or signalling
        is to be sent.

        This procedure is used when the network has downlink signalling
        pending, the UE has uplink signalling pending, the UE or the
        network has user data pending and the UE is in EMM-IDLE mode.

*****************************************************************************/
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "log.h"
#include "msc.h"
#include "dynamic_memory_check.h"
#include "common_types.h"
#include "common_defs.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "mme_app_ue_context.h"
#include "emm_proc.h"
#include "nas_timer.h"
#include "emm_data.h"
#include "emm_sap.h"
#include "emm_cause.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/
static int  _emm_service_reject (void *args);
/*
   --------------------------------------------------------------------------
    Internal data handled by the service request procedure in the UE
   --------------------------------------------------------------------------
*/

/*
   --------------------------------------------------------------------------
    Internal data handled by the service request procedure in the MME
   --------------------------------------------------------------------------
*/


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
/****************************************************************************
 **                                                                        **
 ** Name:        emm_proc_tracking_area_update_reject()                    **
 **                                                                        **
 ** Description:                                                           **
 **                                                                        **
 ** Inputs:  ue_id:              UE lower layer identifier                  **
 **                  emm_cause: EMM cause code to be reported              **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int
emm_proc_service_reject (
  mme_ue_s1ap_id_t ue_id,
  emm_cause_t emm_cause)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;

  /*
   * Create temporary UE context
   */
  struct ue_mm_context_s                      ue_mm_ctx = {0};

  ue_mm_ctx.emm_context.is_dynamic = false;
  ue_mm_ctx.mme_ue_s1ap_id = ue_id;
  /*
   * Update the EMM cause code
   */
  if (ue_id > 0) {
    ue_mm_ctx.emm_context.emm_cause = emm_cause;
  } else {
    ue_mm_ctx.emm_context.emm_cause = EMM_CAUSE_IMPLICITLY_DETACHED;
  }

  /*
   * Do not accept attach request with protocol error
   */
  rc = _emm_service_reject (&ue_mm_ctx);
  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
/** \fn void _emm_tracking_area_update_reject(void *args);
    \brief Performs the tracking area update procedure not accepted by the network.
     @param [in]args UE EMM context data
     @returns status of operation
*/
static int
_emm_service_reject (
  void *args)
{
  OAILOG_FUNC_IN (LOG_NAS_EMM);
  int                                     rc = RETURNerror;
  struct ue_mm_context_s                 *ue_mm_ctx = (struct ue_mm_context_s *) (args);

  if (ue_mm_ctx) {
    emm_sap_t                               emm_sap = {0};

    OAILOG_WARNING (LOG_NAS_EMM, "EMM-PROC  - EMM service procedure not accepted " "by the network (ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n",
    		ue_mm_ctx->mme_ue_s1ap_id, ue_mm_ctx->emm_context.emm_cause);
    /*
     * Notify EMM-AS SAP that Tracking Area Update Reject message has to be sent
     * onto the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REJ;
    emm_sap.u.emm_as.u.establish.ue_id = ue_mm_ctx->mme_ue_s1ap_id;
    emm_sap.u.emm_as.u.establish.eps_id.guti = NULL;

    if (ue_mm_ctx->emm_context.emm_cause == EMM_CAUSE_SUCCESS) {
    	ue_mm_ctx->emm_context.emm_cause = EMM_CAUSE_IMPLICITLY_DETACHED;
    }

    emm_sap.u.emm_as.u.establish.emm_cause = ue_mm_ctx->emm_context.emm_cause;
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_SR;
    emm_sap.u.emm_as.u.establish.nas_msg = NULL;
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data (&emm_sap.u.emm_as.u.establish.sctx, &ue_mm_ctx->emm_context._security, false, false);
    rc = emm_sap_send (&emm_sap);
  }

  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
}
