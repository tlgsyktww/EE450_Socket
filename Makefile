all: serverM.c serverC.c serverCS.c serverEE.c client.c
		gcc -o serverM serverM.c
		gcc -o serverC serverC.c
		gcc -o serverCS serverCS.c
		gcc -o serverEE serverEE.c 
		gcc -o client client.c

serverM:
		./serverM

serverC:
		./serverC

serverCS:
		./serverCS

serverEE:
		./serverEE

.PHONY: serverC serverCS serverEE serverM
