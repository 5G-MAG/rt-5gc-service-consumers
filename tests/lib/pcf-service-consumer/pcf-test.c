/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2023 British Broadcasting Corporation
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

/* Open5GS includes */

#include "ogs-sbi.h"
#include "test-common.h"
#include "af/sbi-path.h"

/* Local library includes */
#include "pcf-service-consumer.h"

/* Unit test includes */
#include "pcf-test-context.h"
#include "pcf-test-sm.h"
#include "pcf-test-media.h"
#include "pcf-test-event-subsc.h"
#include "pcf-test.h"


static ogs_thread_t *thread;
static int initialized = 0;
static ogs_timer_t *t_termination_holding = NULL;

static void pcf_test_main(void *data);
static void event_termination(void);

static bool check_npcf_policyauthortization_af_session_create_result(pcf_app_session_t *result);
static bool check_npcf_policyauthortization_af_session_update_result(pcf_app_session_t *result);
static bool check_npcf_policyauthortization_delete_result(pcf_app_session_t *result);
static bool check_npcf_policyauthortization_evt_subsc_result(pcf_app_session_t *result);

static void test1_func(abts_case *tc, void *data);

int pcf_test_initialise(void)
{
    int rv;

    ogs_sbi_context_init(OpenAPI_nf_type_AF);
    pcf_test_context_init();

    rv = ogs_sbi_context_parse_config("af", "nrf", "scp");
    if (rv != OGS_OK) return rv;

    rv = pcf_test_context_parse_config();
    if (rv != OGS_OK) return rv;

    rv = ogs_log_config_domain(
            ogs_app()->logger.domain, ogs_app()->logger.level);
    if (rv != OGS_OK) return rv;

    thread = ogs_thread_create(pcf_test_main, NULL);
    if (!thread) return OGS_ERROR;

    initialized = 1;

    return OGS_OK;
}

void pcf_test_terminate(void)
{
    if (!initialized) return;

    /* Daemon terminating */
    event_termination();
    ogs_thread_destroy(thread);
    ogs_timer_delete(t_termination_holding);

    pcf_test_context_final();
    ogs_sbi_context_final();

}

abts_suite *test_pcf(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test1_func, NULL);
   
    return suite;
}

static void event_termination(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;

    /* Sending NF Instance De-registeration to NRF */
    ogs_list_for_each(&ogs_sbi_self()->nf_instance_list, nf_instance)
        ogs_sbi_nf_fsm_fini(nf_instance);

    /* Starting holding timer */
    t_termination_holding = ogs_timer_add(ogs_app()->timer_mgr, NULL, NULL);
    ogs_assert(t_termination_holding);
#define TERMINATION_HOLDING_TIME ogs_time_from_msec(300)
    ogs_timer_start(t_termination_holding, TERMINATION_HOLDING_TIME);

    /* Sending termination event to the queue */
    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

static void pcf_test_main(void *data)
{
    ogs_fsm_t pcf_test_sm;
    int rv;

    ogs_fsm_init(&pcf_test_sm, pcf_test_state_initial, pcf_test_state_final, 0);

    for ( ;; ) {
        ogs_pollset_poll(ogs_app()->pollset,
                ogs_timer_mgr_next(ogs_app()->timer_mgr));

        /*
         * After ogs_pollset_poll(), ogs_timer_mgr_expire() must be called.
         *
         * The reason is why ogs_timer_mgr_next() can get the corrent value
         * when ogs_timer_stop() is called internally in ogs_timer_mgr_expire().
         *
         * You should not use event-queue before ogs_timer_mgr_expire().
         * In this case, ogs_timer_mgr_expire() does not work
         * because 'if rv == OGS_DONE' statement is exiting and
         * not calling ogs_timer_mgr_expire().
         */
        ogs_timer_mgr_expire(ogs_app()->timer_mgr);

        for ( ;; ) {
            ogs_event_t *e = NULL;

            rv = ogs_queue_trypop(ogs_app()->queue, (void**)&e);
            ogs_assert(rv != OGS_ERROR);

            if (rv == OGS_DONE)
                goto done;

            if (rv == OGS_RETRY)
                break;

            ogs_assert(e);
            ogs_fsm_dispatch(&pcf_test_sm, e);
            ogs_event_free(e);
        }
    }
done:

    ogs_fsm_fini(&pcf_test_sm, 0);
}

static bool check_npcf_policyauthortization_af_session_create_result(pcf_app_session_t *result)
{
    if (!result  || !result->pcf_app_session_id)
            return false;
    return true;
}


static bool check_npcf_policyauthortization_af_session_update_result(pcf_app_session_t *result)
{
    if (!result  || !result->pcf_app_session_context_updates || !result->pcf_app_session_context_updates_received)
            return false;
    return true;
}


static bool check_npcf_policyauthortization_evt_subsc_result(pcf_app_session_t *result)
{
 if (!result) return false;
 return true; 
}


static bool check_npcf_policyauthortization_delete_result(pcf_app_session_t *result)
{
    	
    if (!result->pcf_app_session_id) 
	    return true;
    return false;
}


static void test1_func(abts_case *tc, void *data)
{    
    int rv;
    ogs_socknode_t *ngap;
    ogs_socknode_t *gtpu;
    ogs_pkbuf_t *gmmbuf;
    ogs_pkbuf_t *gsmbuf;
    ogs_pkbuf_t *nasbuf;
    ogs_pkbuf_t *sendbuf;
    ogs_pkbuf_t *recvbuf;
    ogs_ngap_message_t message;
    char ue_ipv4addr[OGS_ADDRSTRLEN];
    int i = 0;
    pcf_app_session_t *af_pcf_app_session;
    OpenAPI_events_subsc_req_data_t *evt_subsc_req;
    ogs_sbi_server_t *server = NULL;
    ogs_sbi_header_t header;
    char *notif_uri;

    af_pcf_app_session = ogs_calloc(1, sizeof(pcf_app_session_t));

    uint8_t tmp[OGS_HUGE_LEN];
    char *_gtp_payload = "34ff0024"
        "0000000100000085 010002004500001c 0c0b000040015a7a 0a2d00010a2d0002"
        "00000964cd7c291f";

    ogs_nas_5gs_mobile_identity_suci_t mobile_identity_suci;
    test_ue_t *test_ue = NULL;
    test_sess_t *sess = NULL;
    test_bearer_t *qos_flow = NULL;

    af_sess_t *af_sess = NULL;
    af_npcf_policyauthorization_param_t af_param;
    pcf_session_t *pcf_session;
    ogs_sockaddr_t *pcf_address;
    ue_network_identifier_t *ue_connection;
    OpenAPI_list_t *media_component;
    OpenAPI_list_t *events;
    uint16_t port = 7777;

    bson_t *doc = NULL;

    /* Setup Test UE & Session Context */
    memset(&mobile_identity_suci, 0, sizeof(mobile_identity_suci));

    mobile_identity_suci.h.supi_format = OGS_NAS_5GS_SUPI_FORMAT_IMSI;
    mobile_identity_suci.h.type = OGS_NAS_5GS_MOBILE_IDENTITY_SUCI;
    mobile_identity_suci.routing_indicator1 = 0;
    mobile_identity_suci.routing_indicator2 = 0xf;
    mobile_identity_suci.routing_indicator3 = 0xf;
    mobile_identity_suci.routing_indicator4 = 0xf;
    mobile_identity_suci.protection_scheme_id = OGS_PROTECTION_SCHEME_NULL;
    mobile_identity_suci.home_network_pki_value = 0;

    test_ue = test_ue_add_by_suci(&mobile_identity_suci, "0000203190");
    ogs_assert(test_ue);

    test_ue->nr_cgi.cell_id = 0x40001;

    test_ue->nas.registration.tsc = 0;
    test_ue->nas.registration.ksi = OGS_NAS_KSI_NO_KEY_IS_AVAILABLE;
    test_ue->nas.registration.follow_on_request = 1;
    test_ue->nas.registration.value = OGS_NAS_5GS_REGISTRATION_TYPE_INITIAL;

    test_ue->k_string = "465b5ce8b199b49faa5f0a2ee238a6bc";
    test_ue->opc_string = "e8ed289deba952e4283b54e88e6183ca";

    /* gNB connects to AMF */
    ngap = testngap_client(AF_INET);
    ABTS_PTR_NOTNULL(tc, ngap);

    /* gNB connects to UPF */
    gtpu = test_gtpu_server(1, AF_INET);
    ABTS_PTR_NOTNULL(tc, gtpu);

    /* Send NG-Setup Reqeust */
    sendbuf = testngap_build_ng_setup_request(0x4000, 22);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive NG-Setup Response */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /********** Insert Subscriber in Database */
    doc = test_db_new_ims(test_ue);
    ABTS_PTR_NOTNULL(tc, doc);
    ABTS_INT_EQUAL(tc, OGS_OK, test_db_insert_ue(test_ue, doc));

    /* Send Registration request */
    test_ue->registration_request_param.guti = 1;
    gmmbuf = testgmm_build_registration_request(test_ue, NULL, false, false);
    ABTS_PTR_NOTNULL(tc, gmmbuf);

    test_ue->registration_request_param.gmm_capability = 1;
    test_ue->registration_request_param.requested_nssai = 1;
    test_ue->registration_request_param.last_visited_registered_tai = 1;
    test_ue->registration_request_param.ue_usage_setting = 1;
    nasbuf = testgmm_build_registration_request(test_ue, NULL, false, false);
    ABTS_PTR_NOTNULL(tc, nasbuf);

    sendbuf = testngap_build_initial_ue_message(test_ue, gmmbuf,
                NGAP_RRCEstablishmentCause_mo_Signalling, false, true);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive Identity request */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /* Send Identity response */
    gmmbuf = testgmm_build_identity_response(test_ue);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive Authentication request */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /* Send Authentication response */
    gmmbuf = testgmm_build_authentication_response(test_ue);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive Security mode command */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /* Send Security mode complete */
    gmmbuf = testgmm_build_security_mode_complete(test_ue, nasbuf);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive InitialContextSetupRequest +
     * Registration accept */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);
    ABTS_INT_EQUAL(tc,
            NGAP_ProcedureCode_id_InitialContextSetup,
            test_ue->ngap_procedure_code);

    /* Send UERadioCapabilityInfoIndication */
    sendbuf = testngap_build_ue_radio_capability_info_indication(test_ue);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Send InitialContextSetupResponse */
    sendbuf = testngap_build_initial_context_setup_response(test_ue, false);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Send Registration complete */
    gmmbuf = testgmm_build_registration_complete(test_ue);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive Configuration update command */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /* Send PDU session establishment request */
    sess = test_sess_add_by_dnn_and_psi(test_ue, "internet", 5);
    ogs_assert(sess);

    sess->ul_nas_transport_param.request_type =
        OGS_NAS_5GS_REQUEST_TYPE_INITIAL;
    sess->ul_nas_transport_param.dnn = 1;
    sess->ul_nas_transport_param.s_nssai = 1;

    sess->pdu_session_establishment_param.ssc_mode = 1;
    sess->pdu_session_establishment_param.epco = 1;

    gsmbuf = testgsm_build_pdu_session_establishment_request(sess);
    ABTS_PTR_NOTNULL(tc, gsmbuf);
    gmmbuf = testgmm_build_ul_nas_transport(sess,
            OGS_NAS_PAYLOAD_CONTAINER_N1_SM_INFORMATION, gsmbuf);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive PDUSessionResourceSetupRequest +
     * DL NAS transport +
     * PDU session establishment accept */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);
    ABTS_INT_EQUAL(tc,
            NGAP_ProcedureCode_id_PDUSessionResourceSetup,
            test_ue->ngap_procedure_code);

    /* Send GTP-U ICMP Packet */
    qos_flow = test_qos_flow_find_by_qfi(sess, 1);
    ogs_assert(qos_flow);
    rv = test_gtpu_send_ping(gtpu, qos_flow, TEST_PING_IPV4);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Send PDUSessionResourceSetupResponse */
    sendbuf = testngap_sess_build_pdu_session_resource_setup_response(sess);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive GTP-U ICMP Packet */
    recvbuf = testgnb_gtpu_read(gtpu);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    ogs_pkbuf_free(recvbuf);

    /* Send GTP-U ICMP Packet */
    rv = test_gtpu_send_ping(gtpu, qos_flow, TEST_PING_IPV4);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive GTP-U ICMP Packet */
    recvbuf = testgnb_gtpu_read(gtpu);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    ogs_pkbuf_free(recvbuf);

    /* Send GTP-U Router Solicitation */
    rv = test_gtpu_send_slacc_rs(gtpu, qos_flow);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive GTP-U Router Advertisement */
    recvbuf = test_gtpu_read(gtpu);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testgtpu_recv(test_ue, recvbuf);

    /* Send PDU session establishment request */
    sess = test_sess_add_by_dnn_and_psi(test_ue, "ims", 6);
    ogs_assert(sess);

    sess->ul_nas_transport_param.request_type =
        OGS_NAS_5GS_REQUEST_TYPE_INITIAL;
    sess->ul_nas_transport_param.dnn = 1;
    sess->ul_nas_transport_param.s_nssai = 1;

    sess->pdu_session_establishment_param.ssc_mode = 1;
    sess->pdu_session_establishment_param.epco = 1;

    gsmbuf = testgsm_build_pdu_session_establishment_request(sess);
    ABTS_PTR_NOTNULL(tc, gsmbuf);
    gmmbuf = testgmm_build_ul_nas_transport(sess,
            OGS_NAS_PAYLOAD_CONTAINER_N1_SM_INFORMATION, gsmbuf);
    ABTS_PTR_NOTNULL(tc, gmmbuf);
    sendbuf = testngap_build_uplink_nas_transport(test_ue, gmmbuf);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Receive PDU session establishment accept */
    recvbuf = testgnb_ngap_read(ngap);
    ABTS_PTR_NOTNULL(tc, recvbuf);
    testngap_recv(test_ue, recvbuf);

    /* Send PDUSessionResourceSetupResponse */
    sendbuf = testngap_sess_build_pdu_session_resource_setup_response(sess);
    ABTS_PTR_NOTNULL(tc, sendbuf);
    rv = testgnb_ngap_send(ngap, sendbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    /* Add AF-Session */
    af_sess = af_sess_add_by_ue_address(&sess->ue_ip);
    ogs_assert(af_sess);

    af_sess->supi = ogs_strdup(test_ue->supi);
    ogs_assert(af_sess->supi);

    af_sess->dnn = ogs_strdup(sess->dnn);
    ogs_assert(af_sess->dnn);

    /* Wait for PCF-Discovery */
    ogs_msleep(100);

    /* Send AF-Session : CREATE */
    memset(&af_param, 0, sizeof(af_param));
    af_param.med_type = OpenAPI_media_type_AUDIO;
    af_param.qos_type = 1;
    af_param.flow_type = 99; /* For ping test */

    rv = ogs_getaddrinfo(&pcf_address, AF_UNSPEC, "127.0.0.13", port, AI_PASSIVE);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);

    pcf_session = pcf_session_new(pcf_address);
    ogs_assert(pcf_session);
    ogs_freeaddrinfo(pcf_address);
    
    ue_connection = ogs_calloc(1, sizeof(ue_network_identifier_t));
    ogs_assert(ue_connection);
    rv= ogs_ip_to_sockaddr(&sess->ue_ip, port,  &ue_connection->address);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    ue_connection->supi = ogs_strdup(test_ue->supi);
    ogs_assert(ue_connection->supi);
    ue_connection->dnn = ogs_strdup(sess->dnn);
    ogs_assert(ue_connection->dnn);
    
    OGS_ADDR(ue_connection->address, ue_ipv4addr);
    
    media_component = media_component_create(&af_param);

    ogs_msleep(100);

    rv = pcf_session_create_app_session(pcf_session,
                ue_connection, PCF_APP_SESSION_EVENT_TYPE_QOS_NOTIF,
                media_component,
                notification_handler_callback, NULL,
                change_handler_callback, af_pcf_app_session);

    ABTS_INT_EQUAL(tc, 1, rv);
   		
    ogs_msleep(10000);

    ABTS_TRUE(tc, check_npcf_policyauthortization_af_session_create_result(af_pcf_app_session));
    ogs_msleep(100);


    events = tests_events_subsc_create(PCF_APP_SESSION_EVENT_TYPE_QOS_MONITORING, OpenAPI_af_notif_method_ONE_TIME);
    
    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_assert(server);

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OGS_SBI_SERVICE_NAME_NPCF_POLICYAUTHORIZATION;
    header.api.version = (char *)OGS_SBI_API_V1;
    header.resource.component[0] = (char *)OGS_SBI_RESOURCE_NAME_APP_SESSIONS;
    header.resource.component[1] = (char *)af_pcf_app_session->pcf_app_session_id;
    notif_uri = ogs_sbi_server_uri(server, &header);

    evt_subsc_req = OpenAPI_events_subsc_req_data_create(events, notif_uri, NULL, NULL, NULL, NULL, NULL, NULL, true, 0);

    pcf_app_session_subscribe_event(af_pcf_app_session, evt_subsc_req, notification_handler_callback, NULL);
    
    ogs_msleep(10000);

    ABTS_TRUE(tc, check_npcf_policyauthortization_evt_subsc_result(af_pcf_app_session));

    media_component = media_component_create(&af_param);
    ogs_assert(media_component);
    af_pcf_app_session->pcf_app_session_context_updates = NULL;
    af_pcf_app_session->pcf_app_session_context_updates_received = NULL;

    pcf_session_update_app_session(af_pcf_app_session, media_component);

    ogs_msleep(10000);

    ABTS_TRUE(tc, check_npcf_policyauthortization_af_session_update_result(af_pcf_app_session));

    pcf_app_session_unsubscribe_event(af_pcf_app_session, evt_subsc_req);
    
    ogs_msleep(10000);
    
    pcf_app_session_free(af_pcf_app_session);
    ogs_msleep(10000);

    ABTS_TRUE(tc, check_npcf_policyauthortization_delete_result(af_pcf_app_session));

    if(evt_subsc_req) OpenAPI_events_subsc_req_data_free(evt_subsc_req);

    pcf_session_free(pcf_session);
    ogs_msleep(100);
    ogs_free(af_pcf_app_session);


    /* gNB disonncect from UPF */
    testgnb_gtpu_close(gtpu);

    /* gNB disonncect from AMF */
    testgnb_ngap_close(ngap);

    /* Clear Test UE Context */
    test_ue_remove(test_ue);
}

bool notification_handler_callback(pcf_app_session_t *app_session, const OpenAPI_events_notification_t *notifications, void *user_data){
    if (!notifications) return false;
    return true;
}

bool change_handler_callback(pcf_app_session_t *app_session, void *data){
    ogs_debug("change_handler_callback(app_session=%p, data=%p)", app_session, data);
    ogs_assert(app_session);
    if(app_session){
	pcf_app_session_t *result;
        result	= (pcf_app_session_t *)data;
	memcpy(result, app_session, sizeof(pcf_app_session_t));
    }
    if(!app_session || !data) {
	    return false;
    }
    return true;
}




