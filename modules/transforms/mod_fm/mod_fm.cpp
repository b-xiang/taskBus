
const char * g_info = "{\n\
	\"mod_fm\":{\n\
		\"name\":\"mod_fm\",\n\
		\"parameters\":{\n\
			 \"sptype\":{\n\
				\"type\":\"enum\",\n\
				\"tooltip\":\"sample point format\",\n\
				\"default\":0,\n\
				\"range\":{\n\
					\"0\":\"16 bit Intel\",\n\
					\"1\":\"16 bit Moto\",\n\
					\"2\":\"int8\",\n\
					\"3\":\"uint8\"\n\
				}\n\
			},\n\
			 \"channels\":{\n\
				\"type\":\"int\",\n\
				\"tooltip\":\"Channels\",\n\
				\"default\":1\n\
			},\n\
			\"in_spr\":{\n\
			   \"type\":\"int\",\n\
			   \"tooltip\":\"Input Sample Rate\",\n\
			   \"default\":8000\n\
			},\n\
			\"out_spr\":{\n\
			   \"type\":\"int\",\n\
			   \"tooltip\":\"Output Sample Rate\",\n\
			   \"default\":2500000\n\
			}\n\
		},\n\
		\"input_subjects\":\n\
		{\n\
			\"sound\":{\n\
				\"type\":\"byte\",\n\
				\"tooltip\":\"Sound Input\"\n\
			},\n\
			\"tmstamp_in\":{\n\
				\"type\":\"uint64\",\n\
				\"tooltip\":\"tmstamp_in\"\n\
			}\n\
		},\n\
		\"output_subjects\":{\n\
			\"signal\":{\n\
				  \"type\":\"vector\",\n\
				  \"tooltip\":\"FM Signal\"\n\
			 },\n\
			\"tmstamp_out\":{\n\
				\"type\":\"uint64\",\n\
				\"tooltip\":\"tmstamp_out\"\n\
			}\n\
		}\n\
	}\n\
}\n\
";
