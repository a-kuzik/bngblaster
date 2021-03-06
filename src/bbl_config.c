/*
 * BNG Blaster (BBL) - Configuration
 *
 * Hannes Gredler, July 2020
 * Christian Giese, October 2020
 *
 * Copyright (C) 2020-2021, RtBrick, Inc.
 */

#include "bbl.h"
#include "bbl_config.h"
#include <jansson.h>
#include <sys/stat.h>

const char g_default_user[] = "user{session-global}@rtbrick.com";
const char g_default_pass[] = "test";
const char g_default_ari[] = "DEU.RTBRICK.{session-global}";
const char g_default_aci[] = "0.0.0.0/0.0.0.0 eth 0:{session-global}";

static bool
json_parse_access_interface (bbl_ctx_s *ctx, json_t *access_interface, bbl_access_config_s *access_config) {
    json_t *value = NULL;
    const char *s = NULL;
    uint32_t ipv4;

    if (json_unpack(access_interface, "{s:s}", "type", &s) == 0) {
        if (strcmp(s, "pppoe") == 0) {
            access_config->access_type = ACCESS_TYPE_PPPOE;
        } else if (strcmp(s, "ipoe") == 0) {
            access_config->access_type = ACCESS_TYPE_IPOE;
        } else {
            fprintf(stderr, "JSON config error: Invalid value for access->type\n");
            return false;
        }
    }
    if (json_unpack(access_interface, "{s:s}", "interface", &s) == 0) {
        snprintf(access_config->interface, IFNAMSIZ, "%s", s);
    } else {
        fprintf(stderr, "JSON config error: Missing value for access->interface\n");
        return false;
    }

    value = json_object_get(access_interface, "outer-vlan-min");
    if (json_is_number(value)) {
        access_config->access_outer_vlan_min = json_number_value(value);
        access_config->access_outer_vlan_min &= 4095;
    }
    value = json_object_get(access_interface, "outer-vlan-max");
    if (value) {
        access_config->access_outer_vlan_max = json_number_value(value);
        access_config->access_outer_vlan_max &= 4095;
    }
    value = json_object_get(access_interface, "inner-vlan-min");
    if (value) {
        access_config->access_inner_vlan_min = json_number_value(value);
        access_config->access_inner_vlan_min &= 4095;
    }
    value = json_object_get(access_interface, "inner-vlan-max");
    if (value) {
        access_config->access_inner_vlan_max = json_number_value(value);
        access_config->access_inner_vlan_max &= 4095;
    }
    if(access_config->access_outer_vlan_min > access_config->access_outer_vlan_max ||
       access_config->access_inner_vlan_min > access_config->access_inner_vlan_max) {
        fprintf(stderr, "JSON config error: Invalid VLAN range (min > max)\n");
        return false;
    }
    value = json_object_get(access_interface, "third-vlan");
    if (value) {
        access_config->access_third_vlan = json_number_value(value);
        access_config->access_third_vlan &= 4095;
    }

    if (json_unpack(access_interface, "{s:s}", "address", &s) == 0) {
        if(!inet_pton(AF_INET, s, &ipv4)) {
            fprintf(stderr, "JSON config error: Invalid value for access->address\n");
            return false;
        }
        access_config->static_ip = ipv4;
    }
    if (json_unpack(access_interface, "{s:s}", "address-iter", &s) == 0) {
        if(!inet_pton(AF_INET, s, &ipv4)) {
            fprintf(stderr, "JSON config error: Invalid value for access->address-iter\n");
            return false;
        }
        access_config->static_ip_iter = ipv4;
    }
    if (json_unpack(access_interface, "{s:s}", "gateway", &s) == 0) {
        if(!inet_pton(AF_INET, s, &ipv4)) {
            fprintf(stderr, "JSON config error: Invalid value for access->gateway\n");
            return false;
        }
        access_config->static_gateway = ipv4;
    }
    if (json_unpack(access_interface, "{s:s}", "gateway-iter", &s) == 0) {
        if(!inet_pton(AF_INET, s, &ipv4)) {
            fprintf(stderr, "JSON config error: Invalid value for access->gateway-iter\n");
            return false;
        }
        access_config->static_gateway_iter = ipv4;
    }

    /* Optionally overload some settings per range */
    if (json_unpack(access_interface, "{s:s}", "username", &s) == 0) {
        snprintf(access_config->username, USERNAME_LEN, "%s", s);
    } else {
        snprintf(access_config->username, USERNAME_LEN, "%s", ctx->config.username);
    }

    if (json_unpack(access_interface, "{s:s}", "password", &s) == 0) {
        snprintf(access_config->password, PASSWORD_LEN, "%s", s);
    } else {
        snprintf(access_config->password, PASSWORD_LEN, "%s", ctx->config.password);
    }

    if (json_unpack(access_interface, "{s:s}", "authentication-protocol", &s) == 0) {
        if (strcmp(s, "PAP") == 0) {
            access_config->authentication_protocol = PROTOCOL_PAP;
        } else if (strcmp(s, "CHAP") == 0) {
            access_config->authentication_protocol = PROTOCOL_CHAP;
        } else {
            fprintf(stderr, "Config error: Invalid value for access->authentication-protocol\n");
            return false;
        }
    } else {
        access_config->authentication_protocol = ctx->config.authentication_protocol;
    }

    /* Access Line */
    if (json_unpack(access_interface, "{s:s}", "agent-circuit-id", &s) == 0) {
        snprintf(access_config->agent_circuit_id, ACI_LEN, "%s", s);
    } else {
        snprintf(access_config->agent_circuit_id, ACI_LEN, "%s", ctx->config.agent_circuit_id);
    }

    if (json_unpack(access_interface, "{s:s}", "agent-remote-id", &s) == 0) {
        snprintf(access_config->agent_remote_id, ARI_LEN, "%s", s);
    } else {
        snprintf(access_config->agent_remote_id, ARI_LEN, "%s", ctx->config.agent_remote_id);
    }

    value = json_object_get(access_interface, "rate-up");
    if (value) {
        access_config->rate_up = json_number_value(value);
    } else {
        access_config->rate_up = ctx->config.rate_up;
    }

    value = json_object_get(access_interface, "rate-down");
    if (value) {
        access_config->rate_down = json_number_value(value);
    } else {
        access_config->rate_down = ctx->config.rate_down;
    }

    value = json_object_get(access_interface, "ipcp");
    if (json_is_boolean(value)) {
        access_config->ipcp_enable = json_boolean_value(value);
    } else {
        access_config->ipcp_enable = ctx->config.ipcp_enable;
    }
    value = json_object_get(access_interface, "ip6cp");
    if (json_is_boolean(value)) {
        access_config->ip6cp_enable = json_boolean_value(value);
    } else {
        access_config->ip6cp_enable = ctx->config.ip6cp_enable;
    }
    value = json_object_get(access_interface, "ipv4");
    if (json_is_boolean(value)) {
        access_config->ipv4_enable = json_boolean_value(value);
    } else {
        access_config->ipv4_enable = ctx->config.ipv4_enable;
    }
    value = json_object_get(access_interface, "ipv6");
    if (json_is_boolean(value)) {
        access_config->ipv6_enable = json_boolean_value(value);
    } else {
        access_config->ipv6_enable = ctx->config.ipv6_enable;
    }
#if 0
    value = json_object_get(access_interface, "dhcp");
    if (json_is_boolean(value)) {
        access_config->dhcp_enable = json_boolean_value(value);
    } else {
        access_config->dhcp_enable = ctx->config.dhcp_enable;
    }
#endif
    value = json_object_get(access_interface, "dhcpv6");
    if (json_is_boolean(value)) {
        access_config->dhcpv6_enable = json_boolean_value(value);
    } else {
        access_config->dhcpv6_enable = ctx->config.dhcpv6_enable;
    }
    value = json_object_get(access_interface, "igmp-autostart");
    if (json_is_boolean(value)) {
        access_config->igmp_autostart = json_boolean_value(value);
    } else {
        access_config->igmp_autostart = ctx->config.igmp_autostart;
    }
    value = json_object_get(access_interface, "igmp-version");
    if (json_is_number(value)) {
        access_config->igmp_version = json_number_value(value);
        if(access_config->igmp_version < 1 || access_config->igmp_version > 3) {
            fprintf(stderr, "JSON config error: Invalid value for access->igmp-version\n");
            return false;
        }
    } else {
        access_config->igmp_version = ctx->config.igmp_version;
    }
    value = json_object_get(access_interface, "session-traffic-autostart");
    if (json_is_boolean(value)) {
        access_config->session_traffic_autostart = json_boolean_value(value);
    } else {
        access_config->session_traffic_autostart = ctx->config.session_traffic_autostart;
    }
    return true;
}

static bool
json_parse_config (json_t *root, bbl_ctx_s *ctx) {

    json_t *section, *sub, *value = NULL;
    const char *s;
    uint32_t ipv4;
    int i, size;

    bbl_access_config_s *access_config = NULL;

    if(json_typeof(root) != JSON_OBJECT) {
        fprintf(stderr, "JSON config error: Configuration root element must object\n");
        return false;
    }

    /* Sessions Configuration */
    section = json_object_get(root, "sessions");
    if (json_is_object(section)) {
        value = json_object_get(section, "count");
        if (json_is_number(value)) {
            ctx->config.sessions = json_number_value(value);
        }
        value = json_object_get(section, "max-outstanding");
        if (json_is_number(value)) {
            ctx->config.sessions_max_outstanding = json_number_value(value);
        }
        value = json_object_get(section, "start-rate");
        if (json_is_number(value)) {
            ctx->config.sessions_start_rate = json_number_value(value);
        }
        value = json_object_get(section, "stop-rate");
        if (json_is_number(value)) {
            ctx->config.sessions_stop_rate = json_number_value(value);
        }
    }

    /* IPoE Configuration */
    section = json_object_get(root, "ipoe");
    if (json_is_object(section)) {
        value = json_object_get(section, "ipv4");
        if (json_is_boolean(value)) {
            ctx->config.ipv4_enable = json_boolean_value(value);
        }
        value = json_object_get(section, "ipv6");
        if (json_is_boolean(value)) {
            ctx->config.ipv6_enable = json_boolean_value(value);
        }
    }

    /* PPPoE Configuration */
    section = json_object_get(root, "pppoe");
    if (json_is_object(section)) {
        /* Deprecated ... 
         * PPPoE sessions, max-outstanding, start
         * and stop rate was moved to section session
         * as all those values apply to PPPoE and IPoE
         * but for compatibility they are still supported
         * here as well for some time. 
         */
        value = json_object_get(section, "sessions");
        if (json_is_number(value)) {
            ctx->config.sessions = json_number_value(value);
        }
        value = json_object_get(section, "max-outstanding");
        if (json_is_number(value)) {
            ctx->config.sessions_max_outstanding = json_number_value(value);
        }
        value = json_object_get(section, "start-rate");
        if (json_is_number(value)) {
            ctx->config.sessions_start_rate = json_number_value(value);
        }
        value = json_object_get(section, "stop-rate");
        if (json_is_number(value)) {
            ctx->config.sessions_stop_rate = json_number_value(value);
        }
        /* ... Deprecated */

        value = json_object_get(section, "session-time");
        if (json_is_number(value)) {
            ctx->config.pppoe_session_time = json_number_value(value);
        }
        value = json_object_get(section, "reconnect");
        if (json_is_boolean(value)) {
            ctx->config.pppoe_reconnect = json_boolean_value(value);
        }
        value = json_object_get(section, "discovery-timeout");
        if (json_is_number(value)) {
            ctx->config.pppoe_discovery_timeout = json_number_value(value);
        }
        value = json_object_get(section, "discovery-retry");
        if (json_is_number(value)) {
            ctx->config.pppoe_discovery_retry = json_number_value(value);
        }
    }

    /* PPP Configuration */
    section = json_object_get(root, "ppp");
    if (json_is_object(section)) {
        value = json_object_get(section, "mru");
        if (json_is_number(value)) {
            ctx->config.ppp_mru = json_number_value(value);
        }
        sub = json_object_get(section, "authentication");
        if (json_is_object(sub)) {
            if (json_unpack(sub, "{s:s}", "username", &s) == 0) {
                snprintf(ctx->config.username, USERNAME_LEN, "%s", s);   
            }
            if (json_unpack(sub, "{s:s}", "password", &s) == 0) {
                snprintf(ctx->config.password, PASSWORD_LEN, "%s", s);
            }
            value = json_object_get(sub, "timeout");
            if (json_is_number(value)) {
                ctx->config.authentication_timeout = json_number_value(value);
            }
            value = json_object_get(sub, "retry");
            if (json_is_number(value)) {
                ctx->config.authentication_retry = json_number_value(value);
            }
            if (json_unpack(sub, "{s:s}", "protocol", &s) == 0) {
                if (strcmp(s, "PAP") == 0) {
                    ctx->config.authentication_protocol = PROTOCOL_PAP;
                } else if (strcmp(s, "CHAP") == 0) {
                    ctx->config.authentication_protocol = PROTOCOL_CHAP;
                } else {
                    fprintf(stderr, "JSON config error: Invalid value for ppp->authentication->protocol\n");
                    return false;
                }
            }
        }
        sub = json_object_get(section, "lcp");
        if (json_is_object(sub)) {
            value = json_object_get(sub, "conf-request-timeout");
            if (json_is_number(value)) {
                ctx->config.lcp_conf_request_timeout = json_number_value(value);
            }
            value = json_object_get(sub, "conf-request-retry");
            if (json_is_number(value)) {
                ctx->config.lcp_conf_request_retry = json_number_value(value);
            }
            value = json_object_get(sub, "keepalive-interval");
            if (json_is_number(value)) {
                ctx->config.lcp_keepalive_interval = json_number_value(value);
            }
            value = json_object_get(sub, "keepalive-retry");
            if (json_is_number(value)) {
                ctx->config.lcp_keepalive_retry = json_number_value(value);
            }
        }
        sub = json_object_get(section, "ipcp");
        if (json_is_object(sub)) {
            value = json_object_get(sub, "enable");
            if (json_is_boolean(value)) {
                ctx->config.ipcp_enable = json_boolean_value(value);
            }
            value = json_object_get(sub, "request-ip");
            if (json_is_boolean(value)) {
                ctx->config.ipcp_request_ip = json_boolean_value(value);
            }
            value = json_object_get(sub, "request-dns1");
            if (json_is_boolean(value)) {
                ctx->config.ipcp_request_dns1 = json_boolean_value(value);
            }
            value = json_object_get(sub, "request-dns2");
            if (json_is_boolean(value)) {
                ctx->config.ipcp_request_dns2 = json_boolean_value(value);
            }
            value = json_object_get(sub, "conf-request-timeout");
            if (json_is_number(value)) {
                ctx->config.ipcp_conf_request_timeout = json_number_value(value);
            }
            value = json_object_get(sub, "conf-request-retry");
            if (json_is_number(value)) {
                ctx->config.ipcp_conf_request_retry = json_number_value(value);
            }
        }
        sub = json_object_get(section, "ip6cp");
        if (json_is_object(sub)) {
            value = json_object_get(sub, "enable");
            if (json_is_boolean(value)) {
                ctx->config.ip6cp_enable = json_boolean_value(value);
            }
            value = json_object_get(sub, "conf-request-timeout");
            if (json_is_number(value)) {
                ctx->config.ip6cp_conf_request_timeout = json_number_value(value);
            }
            value = json_object_get(sub, "conf-request-retry");
            if (json_is_number(value)) {
                ctx->config.ip6cp_conf_request_retry = json_number_value(value);
            }
        }
    }
#if 0
    /* DHCP Configuration */
    section = json_object_get(root, "dhcp");
    if (json_is_object(section)) {
        value = json_object_get(section, "enable");
        if (json_is_boolean(value)) {
            ctx->config.dhcp_enable = json_boolean_value(value);
        }
    }
#endif

    /* DHCPv6 Configuration */
    section = json_object_get(root, "dhcpv6");
    if (json_is_object(section)) {
        value = json_object_get(section, "enable");
        if (json_is_boolean(value)) {
            ctx->config.dhcpv6_enable = json_boolean_value(value);
        }
        value = json_object_get(section, "rapid-commit");
        if (json_is_boolean(value)) {
            ctx->config.dhcpv6_rapid_commit = json_boolean_value(value);
        }
    }

    /* IGMP Configuration */
    section = json_object_get(root, "igmp");
    if (json_is_object(section)) {
        value = json_object_get(section, "version");
        if (json_is_number(value)) {
            ctx->config.igmp_version = json_number_value(value);
            if(ctx->config.igmp_version < 1 || ctx->config.igmp_version > 3) {
                fprintf(stderr, "JSON config error: Invalid value for igmp->version\n");
                return false;
            }
        }
        value = json_object_get(section, "combined-leave-join");
        if (json_is_boolean(value)) {
            ctx->config.igmp_combined_leave_join = json_boolean_value(value);
        }
        value = json_object_get(section, "autostart");
        if (json_is_boolean(value)) {
            ctx->config.igmp_autostart = json_boolean_value(value);
        }
        value = json_object_get(section, "start-delay");
        if (json_is_number(value) && json_number_value(value)) {
            /* Min 1 second */
            ctx->config.igmp_start_delay = json_number_value(value);
        }
        if (json_unpack(section, "{s:s}", "group", &s) == 0) {
            if(!inet_pton(AF_INET, s, &ipv4)) {
                fprintf(stderr, "JSON config error: Invalid value for igmp->group\n");
                return false;
            }
            ctx->config.igmp_group = ipv4;
        }
        if (json_unpack(section, "{s:s}", "group-iter", &s) == 0) {
            if(!inet_pton(AF_INET, s, &ipv4)) {
                fprintf(stderr, "JSON config error: Invalid value for igmp->group-iter\n");
                return false;
            }
            ctx->config.igmp_group_iter = ipv4;
        }
        if (json_unpack(section, "{s:s}", "source", &s) == 0) {
            if(!inet_pton(AF_INET, s, &ipv4)) {
                fprintf(stderr, "JSON config error: Invalid value for igmp->source\n");
                return false;
            }
            ctx->config.igmp_source = ipv4;
        }
        value = json_object_get(section, "group-count");
        if (json_is_number(value)) {
            ctx->config.igmp_group_count = json_number_value(value);
        }
        value = json_object_get(section, "zapping-interval");
        if (json_is_number(value)) {
            ctx->config.igmp_zap_interval = json_number_value(value);
        }
        value = json_object_get(section, "zapping-view-duration");
        if (json_is_number(value)) {
            ctx->config.igmp_zap_view_duration = json_number_value(value);
        }
        value = json_object_get(section, "zapping-count");
        if (json_is_number(value)) {
            ctx->config.igmp_zap_count = json_number_value(value);
        }
        value = json_object_get(section, "zapping-wait");
        if (json_is_boolean(value)) {
            ctx->config.igmp_zap_wait = json_boolean_value(value);
        }
        value = json_object_get(section, "send-multicast-traffic");
        if (json_is_boolean(value)) {
            ctx->config.send_multicast_traffic = json_boolean_value(value);
        }
    }

    /* Access Line Configuration */
    section = json_object_get(root, "access-line");
    if (json_is_object(section)) {
        if (json_unpack(section, "{s:s}", "agent-circuit-id", &s) == 0) {
            snprintf(ctx->config.agent_circuit_id, ACI_LEN, "%s", s );
        }
        if (json_unpack(section, "{s:s}", "agent-remote-id", &s) == 0) {
            snprintf(ctx->config.agent_remote_id, ARI_LEN, "%s", s );
        }
        value = json_object_get(section, "rate-up");
        if (json_is_number(value)) {
            ctx->config.rate_up = json_number_value(value);
        }
        value = json_object_get(section, "rate-down");
        if (json_is_number(value)) {
            ctx->config.rate_down = json_number_value(value);
        }
    }

    /* Session Traffic Configuration */
    section = json_object_get(root, "session-traffic");
    if (json_is_object(section)) {
        value = json_object_get(section, "autostart");
        if (json_is_boolean(value)) {
            ctx->config.session_traffic_autostart = json_boolean_value(value);
        }
        value = json_object_get(section, "ipv4-pps");
        if (json_is_number(value)) {
            ctx->config.session_traffic_ipv4_pps = json_number_value(value);
        }
        value = json_object_get(section, "ipv6-pps");
        if (json_is_number(value)) {
            ctx->config.session_traffic_ipv6_pps = json_number_value(value);
        }
        value = json_object_get(section, "ipv6pd-pps");
        if (json_is_number(value)) {
            ctx->config.session_traffic_ipv6pd_pps = json_number_value(value);
        }
    }

    /* Interface Configuration */
    section = json_object_get(root, "interfaces");
    if (json_is_object(section)) {
        value = json_object_get(section, "tx-interval");
        if (json_is_number(value)) {
            ctx->config.tx_interval = json_number_value(value);
        }
        value = json_object_get(section, "rx-interval");
        if (json_is_number(value)) {
            ctx->config.rx_interval = json_number_value(value);
        }
        sub = json_object_get(section, "network");
        if (json_is_object(sub)) {
            if (json_unpack(sub, "{s:s}", "interface", &s) == 0) {
                snprintf(ctx->config.network_if, IFNAMSIZ, "%s", s);
            }
            if (json_unpack(sub, "{s:s}", "address", &s) == 0) {
                if(!inet_pton(AF_INET, s, &ipv4)) {
                    fprintf(stderr, "JSON config error: Invalid value for network->address\n");
                    return false;
                }
                ctx->config.network_ip = ipv4;
            }
            if (json_unpack(sub, "{s:s}", "gateway", &s) == 0) {
                if(!inet_pton(AF_INET, s, &ipv4)) {
                    fprintf(stderr, "JSON config error: Invalid value for network->gateway\n");
                    return false;
                }
                ctx->config.network_gateway= ipv4;
            }
            if (json_unpack(sub, "{s:s}", "address-ipv6", &s) == 0) {
                if(!inet_pton(AF_INET6, s, &ctx->config.network_ip6.address)) {
                    fprintf(stderr, "JSON config error: Invalid value for network->address-ipv6\n");
                    return false;
                }
                ctx->config.network_ip6.len = 64;
            }
            if (json_unpack(sub, "{s:s}", "gateway-ipv6", &s) == 0) {
                if(!inet_pton(AF_INET6, s, &ctx->config.network_gateway6.address)) {
                    fprintf(stderr, "JSON config error: Invalid value for network->gateway-ipv6\n");
                    return false;
                }
                ctx->config.network_gateway6.len = 64;
            }
            value = json_object_get(sub, "vlan");
            if (json_is_number(value)) {
                ctx->config.network_vlan = json_number_value(value);
                ctx->config.network_vlan &= 4095;
            }
        }
        sub = json_object_get(section, "access");
        if (json_is_array(sub)) {
            /* Config is provided as array (multiple access ranges) */ 
            size = json_array_size(sub);
            for (i = 0; i < size; i++) {
                if(!access_config) {
                    ctx->config.access_config = malloc(sizeof(bbl_access_config_s));
                    access_config = ctx->config.access_config;
                } else {
                    access_config->next = malloc(sizeof(bbl_access_config_s));
                    access_config = access_config->next;
                }
                memset(access_config, 0x0, sizeof(bbl_access_config_s));
                if(!json_parse_access_interface(ctx, json_array_get(sub, i), access_config)) {
                    return false;
                }
            }
        } else if (json_is_object(sub)) {
            /* Config is provided as object (single access range) */
            access_config = malloc(sizeof(bbl_access_config_s));
            memset(access_config, 0x0, sizeof(bbl_access_config_s));
            if(!ctx->config.access_config) {
                ctx->config.access_config = access_config;
            }
            if(!json_parse_access_interface(ctx, sub, access_config)) {
                return false;
            }
        }
    } else {
        fprintf(stderr, "JSON config error: Missing interfaces section\n");
        return false;
    }
    return true;
}

/* bbl_config_load_json
 *
 * This functions is population the BBL context
 * from given JSON configuration file returning
 * true is successfull or false if failed with 
 * error message printed to stderr. 
 */
bool
bbl_config_load_json (char *filename, bbl_ctx_s *ctx) {
    json_t *root = NULL;
    json_error_t error;
    bool result = false;

    root = json_load_file(filename, 0, &error);
    if(root) {
        result = json_parse_config(root, ctx);
        json_decref(root);
    } else {
        fprintf(stderr, "JSON config error: File %s Line %d: %s\n", filename, error.line, error.text);
    }
    return result;
}

/* bbl_config_load_json
 *
 * This functions is population the BBL context
 * with default configuration values. 
 */
void
bbl_config_init_defaults (bbl_ctx_s *ctx) {
    snprintf(ctx->config.username, USERNAME_LEN, "%s", g_default_user);
    snprintf(ctx->config.password,  PASSWORD_LEN, "%s", g_default_pass);
    snprintf(ctx->config.agent_remote_id, ARI_LEN, "%s", g_default_ari);
    snprintf(ctx->config.agent_circuit_id, ACI_LEN, "%s", g_default_aci);
    ctx->config.tx_interval = 5;
    ctx->config.rx_interval = 5;
    ctx->config.sessions = 1;
    ctx->config.sessions_max_outstanding = 800;
    ctx->config.sessions_start_rate = 400,
    ctx->config.sessions_stop_rate = 400,
    ctx->config.pppoe_discovery_timeout = 5;
    ctx->config.pppoe_discovery_retry = 10;
    ctx->config.ppp_mru = 1492;
    ctx->config.lcp_conf_request_timeout = 5;
    ctx->config.lcp_conf_request_retry = 10;
    ctx->config.lcp_keepalive_interval = 30;
    ctx->config.lcp_keepalive_retry = 3;
    ctx->config.authentication_timeout = 5;
    ctx->config.authentication_retry = 30;
    ctx->config.ipv4_enable = true;
    ctx->config.ipv6_enable = true;
    ctx->config.ipcp_enable = true;
    ctx->config.ipcp_request_ip = true;
    ctx->config.ipcp_request_dns1 = true;
    ctx->config.ipcp_request_dns2 = true;
    ctx->config.ipcp_conf_request_timeout = 5;
    ctx->config.ipcp_conf_request_retry = 10;
    ctx->config.ip6cp_enable = true;
    ctx->config.ip6cp_conf_request_timeout = 5;
    ctx->config.ip6cp_conf_request_retry = 10;
    ctx->config.dhcpv6_enable = true;
    ctx->config.dhcpv6_rapid_commit = true;
    ctx->config.igmp_autostart = true;
    ctx->config.igmp_version = IGMP_VERSION_3;
    ctx->config.igmp_start_delay = 1;
    ctx->config.igmp_group = 0;
    ctx->config.igmp_group_iter = htobe32(1);
    ctx->config.igmp_source = 0;
    ctx->config.igmp_group_count = 1;
    ctx->config.igmp_zap_wait = true;
    ctx->config.session_traffic_autostart = true;
}
