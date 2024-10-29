#include "packet_dropper.h"
#include <windows.h>
#include <fwptypes.h>
#include <fwpmtypes.h>
#include <fwpmu.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "macros/macros.h"

static struct
{	wchar_t * direction;
	wchar_t * internet_protocol_version;
	bool requires_ipv6;
	GUID filter_guid;
	GUID layer_guid;
} filters_data[] =
{	{	.direction = L"INBOUND",
		.internet_protocol_version = L"IPV4",
		.requires_ipv6 = false,
		.filter_guid = { 0x67d33cea, 0x1fbc, 0x4b2e, { 0x9f, 0xf, 0x12, 0x2, 0x8b, 0xef, 0x7, 0xd9 } },
		/* FWPM_LAYER_INBOUND_TRANSPORT_V4 */
		.layer_guid = { 0x5926dfc8, 0xe3cf, 0x4426, { 0xa2, 0x83, 0xdc, 0x39, 0x3f, 0x5d, 0x0f, 0x9d } }
	},
	{	.direction = L"OUTBOUND",
	 	.internet_protocol_version = L"IPV4",
		.requires_ipv6 = false,
	 	.filter_guid = { 0xe3516da4, 0x9250, 0x46f6, { 0x9c, 0xc3, 0x5a, 0x6f, 0xdc, 0x34, 0x51, 0x18 } },
		/* FWPM_LAYER_OUTBOUND_TRANSPORT_V4 */
	 	.layer_guid = { 0x09e61aea, 0xd214, 0x46e2, { 0x9b, 0x21, 0xb2, 0x6b, 0x0b, 0x2f, 0x28, 0xc8 } }
	},
	{	.direction = L"INBOUND",
	 	.internet_protocol_version = L"IPV6",
		.requires_ipv6 = true,
	 	.filter_guid = { 0xf5013dff, 0x7d9e, 0x492d, { 0xb4, 0x1, 0x38, 0x71, 0xb9, 0x22, 0xe9, 0xeb } },
	 	/* FWPM_LAYER_INBOUND_TRANSPORT_V6 */
	 	.layer_guid = { 0x634a869f, 0xfc23, 0x4b90, { 0xb0, 0xc1, 0xbf, 0x62, 0x0a, 0x36, 0xae, 0x6f } }
	},
	{	.direction = L"OUTBOUND",
	 	.internet_protocol_version = L"IPV6",
		.requires_ipv6 = true,
	 	.filter_guid = { 0x9575502c, 0x282d, 0x4225, { 0x85, 0x51, 0xf0, 0xc4, 0xd5, 0x6e, 0x4a, 0x9e } },
	 	/* FWPM_LAYER_OUTBOUND_TRANSPORT_V6 */
	 	.layer_guid = { 0xe1735bde, 0x013f, 0x4655, { 0xb3, 0x51, 0xa4, 0x9e, 0x15, 0x76, 0x2d, 0xf0 } }
	}
};

enum { filters_data_length = sizeof(filters_data) / sizeof(filters_data[0]) };

static HANDLE filtering_engine;

void packet_dropper_setup(void)
{	assert(!FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &filtering_engine));
	packet_dropper_stop();
}

void packet_dropper_start(void)
{	for(int i = 0; i < filters_data_length; ++i)
	{	enum { name_size = 50 };
		wchar_t name[name_size];
		wchar_t format[] = L"Replay Glitch Helper 2 WFP Filter %ws %ws";
		int name_length = snwprintf(NULL, 0, format, filters_data[i].direction, filters_data[i].internet_protocol_version);
		assert(name_length > 0 && name_length < (name_size - 1));
		assert(snwprintf(name, name_length + 1, format, filters_data[i].direction, filters_data[i].internet_protocol_version) == name_length);
		FWPM_FILTER0 filter =
		{	.filterKey = filters_data[i].filter_guid,
			.displayData =
			{	.name = name,
				.description = L"https://github.com/CesarBerriot/rgh2"
			},
			.layerKey = filters_data[i].layer_guid,
			.subLayerKey = IID_NULL,
			.weight =
			{	.type = FWP_UINT64,
				.uint64 = (UINT64*)(UINT32[2]){ UINT32_MAX, UINT32_MAX },
			},
			.action.type = FWP_ACTION_BLOCK
		};
		assert(!FwpmFilterAdd0(filtering_engine, &filter, NULL, NULL));
	}
}

void packet_dropper_stop(void)
{	for(int i = 0; i < filters_data_length; ++i)
	{	switch(FwpmFilterDeleteByKey0(filtering_engine, &filters_data[i].filter_guid))
		{	case ERROR_SUCCESS:
			case FWP_E_FILTER_NOT_FOUND:
				break;
			default:
				verbose_abort("failed to delete a WFP filter.");
		}
	}
}

void packet_dropper_cleanup(void)
{	packet_dropper_stop();
	assert(!FwpmEngineClose0(filtering_engine));
}
