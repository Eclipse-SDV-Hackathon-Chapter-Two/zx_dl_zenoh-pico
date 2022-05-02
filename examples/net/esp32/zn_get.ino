/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <Arduino.h>
#include <WiFi.h>

extern "C"
{
#include "zenoh-pico.h"
}

#define SSID "SSID"
#define PASS "PASSWORD"

// Zenoh-specific parameters
#define MODE "client"
#define PEER "tcp/10.0.0.1:7447"
#define URI "/demo/example/**"

_z_session_t *s = NULL;

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    _z_config_t *config = _z_config_default();
    _z_config_insert(config, Z_CONFIG_MODE_KEY, z_string_make(MODE));
    _z_config_insert(config, Z_CONFIG_PEER_KEY, z_string_make(PEER));

    s = _z_open(config);
    if (s == NULL)
        return;

    _zp_start_read_task(s);
    _zp_start_lease_task(s);
}

void loop()
{
    delay(5000);

    if (s == NULL)
        return;

    _z_reply_data_array_t replies = _z_query_collect(s, _z_rname(URI), "", _z_query_target_default(), z_query_consolidation_default());
    for (unsigned int i = 0; i < replies.len; i++)
    {
        // printf(">> [Reply handler] received (%.*s, %.*s)\n",
        //        (int)replies.val[i].data.key.len, replies.val[i].data.key.val,
        //        (int)replies.val[i].data.value.len, replies.val[i].data.value.val);
    }
    _z_reply_data_array_clear(&replies);
}
