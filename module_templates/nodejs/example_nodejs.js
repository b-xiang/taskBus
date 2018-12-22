/* Node.js using non-blocking event system,
 * that is different from C
 * */
//get cmdline args

//Set encoding as utf-8 to recieve asian chars
process.stdin.setEncoding('utf8');

var argu = {};
for (let j = 0; j < process.argv.length; j++) {  
    ar = process.argv[j].split('=');
    if (ar.length==2)
		argu[ar[0]] = ar[1];
	else if (ar.length==1)
		argu[ar[0]] = "";
}
//commandline items
var s_function = argu["--function"];
var s_input = Number(argu["--input"]);
var s_output = Number(argu["--output"]);
var s_instance = Number(argu["--instance"]);
console.error(s_function);
console.error(s_input);
console.error(s_output);
if (argu.hasOwnProperty("--information"))
{
	console.error("Instance Query, Please using json file instead.");
	process.exit();
}
else if (s_function=="example_nodejs")
{
	//Set encoding, ASCII will be good at bytes transfer.
	//UTF-8 is not good
	process.stdin.setEncoding('ascii');

	var events = require('events');
	var eventEmitter = new events.EventEmitter();
	var queue = "";
	//package callback for find valid packages
	eventEmitter.on('package', (package) => {		
		//get header
		queue = queue + package;
		hit = false;
		while (queue.length>=4 && hit == false)
		{
			if (queue[0]=='\x3c' && queue[1]=='\x5A' &&queue[2]=='\x7E' && queue[3]=='\x69')
				hit = true;				
			else
				queue = queue.substring(1,queue.length);
		}
		i_subject = -1;
		i_path = -1;
		i_length = -1;	
		if (queue.length>=16)
		{
			i_subject = queue.charCodeAt(4)+(queue.charCodeAt(5)<<8)+(queue.charCodeAt(6)<<16)+(queue.charCodeAt(7)<<24);
			i_path    = queue.charCodeAt(8)+(queue.charCodeAt(9)<<8)+(queue.charCodeAt(10)<<16)+(queue.charCodeAt(11)<<24);
			i_length  = queue.charCodeAt(12)+(queue.charCodeAt(13)<<8)+(queue.charCodeAt(14)<<16)+(queue.charCodeAt(15)<<24);
		}
		if (queue.length>=i_length+16)
		{
			iheader =  queue.substring(0,4);
			i_data = queue.substring(16,i_length+16);
			queue = queue.substring(i_length+16,queue.length);
			if (s_input==i_subject)
				eventEmitter.emit('dealpack',iheader,i_subject,i_path,i_data);    
			else if (i_subject==0xffffffff)
			{
                if (i_data.indexOf("=quit;")>0)
					process.exit(0);
			}
		}			
	
	});
	//dealpack callback to deal whole package
	eventEmitter.on('dealpack', (iheader,subid,pathid,data) => {
		process.stdout.write(iheader);
		process.stdout.write(String.fromCharCode(
			(s_output&0xff),
			((s_output>>8)&0xff),
			((s_output>>16)&0xff),
			((s_output>>24)&0xff)
			));
		process.stdout.write(String.fromCharCode(
			(pathid&0xff),
			((pathid>>8)&0xff),
			((pathid>>16)&0xff),
			((pathid>>24)&0xff)
			));
		len = data.length;
		process.stdout.write(String.fromCharCode(
			(len&0xff),
			((len>>8)&0xff),
			((len>>16)&0xff),
			((len>>24)&0xff)
			));
		process.stdout.write(data);
		
	});
	
	//Read Callback
	process.stdin.on('readable', () => {
	  var chunk = process.stdin.read();
	  if (chunk !== null) {
		eventEmitter.emit('package',chunk);    
	  }
	});
	console.error("node.js started.");
}
else
	console.error("function name is not example_nodejs");


