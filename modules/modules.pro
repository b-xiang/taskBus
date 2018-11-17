TEMPLATE = subdirs

#main framework project
SUBDIRS  += \ 
    sources/source_soundcard \
    transforms/transform_fft \
    sinks/sink_plots \
    sources/source_files \
    sinks/sink_file \
    network/network_p2p \
    sinks/sink_SQL \
    wrappers/wrapper_scripts



