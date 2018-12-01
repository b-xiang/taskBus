import sys
import tbinterface as tb
if __name__ == "__main__":
    tb.init_client()
    args = tb.args2dict(sys.argv)
    print(args,file=sys.stderr,flush=True)
    if "--information" in args:
        sys.exit(0)
    elif "--function" in args:
        id_input = int(args['--input'][0])
        id_output = int(args['--output'][0])
        id_ins = int(args['--instance'][0])
        finished = False
        while finished == False:
            package = tb.taskBus_pull()
            v_subject = package['subject']
            v_path =  package['path']
            v_bytea = package['package']
            if (v_subject == id_input):
                #deal
                #...
                #push
                tb.taskBus_push(id_output,v_path,v_bytea)
            elif v_subject == 0xffffffff:
		if v_bytea.find(b'function=quit;')>=0:
                    print('Python Script will quit!',file=sys.stderr,flush=True)
                    finished = True

            
    else:
        sys.exit(0)
    
