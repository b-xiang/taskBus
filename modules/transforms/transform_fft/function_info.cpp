
const char * g_info = "{\n\
	\"transform_fft\":{\n\
		\"name\":\"libfftw\",\n\
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
			\"fftsize\":{\n\
				\"type\":\"int\",\n\
				\"tooltip\":\"fft size\",\n\
				\"default\":1024\n\
			}\n\
		},\n\
		\"input_subjects\":\n\
		{\n\
			\"signal\":{\n\
				\"type\":\"byte\",\n\
				\"tooltip\":\"signal\"\n\
			},\n\
			\"tmstamp_in\":{\n\
				\"type\":\"uint64\",\n\
				\"tooltip\":\"tmstamp_in\"\n\
			}\n\
		},\n\
		\"output_subjects\":{\n\
			 \"FFT\":{\n\
				\"type\":\"vector\",\n\
				\"tooltip\":\"FFT in dB\"\n\
			},\n\
			 \"Spec\":{\n\
				  \"type\":\"vector\",\n\
				  \"tooltip\":\"Spec in Complex\"\n\
			 },\n\
			\"tmstamp_out\":{\n\
				\"type\":\"uint64\",\n\
				\"tooltip\":\"tmstamp_out\"\n\
			}\n\
		}\n\
	}\n\
}\n\
";
