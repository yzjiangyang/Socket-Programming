# comment
# (note: the <tab> in the command line is necessary for make to work) 
all:	scheduler hospitalA hospitalB hospitalC client
	

for example:
#
# target entry to build program executable from program and mylib 
# object files 
#
scheduler: 
	gcc -o scheduler scheduler.c

hospitalA: 
	gcc -o hospitalA hospitalA.c
	
hospitalB: 
	gcc -o hospitalB hospitalB.c
	
hospitalC: 
	gcc -o hospitalC hospitalC.c

client: 
	gcc -o client client.c
	
clean:
	rm scheduler
	rm hospitalA
	rm hospitalB
	rm hospitalC
	rm client

