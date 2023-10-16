// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include "parameter.h"
#include "throughputmanagement.h"
#include "udpstream.h"

#define CONNS_ESTAB_TIMEOUT 1200 /* the max time in seconds for sender to establish the connections */