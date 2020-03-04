TEMPLATE = subdirs

#main framework project
SUBDIRS  += sources/source_plutosdr \
    sources/source_soundcard \
    transforms/transform_fft \
    transforms/mod_fm \
    sources/source_files \
    sinks/sink_file \
    sinks/sink_soundcard \
    sinks/sink_plutosdr \
    network/network_p2p \
    sinks/sink_SQL \
    wrappers/wrapper_scripts


qtHaveModule(charts){
SUBDIRS  += \
    sinks/sink_plots
}
