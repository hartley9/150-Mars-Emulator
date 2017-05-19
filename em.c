#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define XSTR(x) STR(x) 
#define STR(x) #x 

#define MAX_PROG_LEN 250
#define MAX_LINE_LEN 50
#define MAX_OPCODE   8 
#define MAX_REGISTER 32 
#define MAX_ARG_LEN  20 

#define ADDR_TEXT    0x00400000 
#define TEXT_POS(a)  ((a==ADDR_TEXT)?(0):(a - ADDR_TEXT)/4)

const char *register_str[] = {	"$zero", 
				"$at",
				"$v0", "$v1",
				"$a0", "$a1", "$a2", "$a3",
				"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
				"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
				"$t8", "$t9",
				"$k0", "$k1",
				"$gp",
				"$sp",
				"$fp",
				"$ra"
				};

unsigned int registers[MAX_REGISTER] = {0};
unsigned int pc = 0;
unsigned int text[MAX_PROG_LEN] = {0};

typedef int (*opcode_function)(unsigned int, unsigned int*, char*, char*, char*, char*);


char prog[MAX_PROG_LEN][MAX_LINE_LEN];
int prog_len=0;


int print_registers(){
	int i;
	printf("registers:\n"); 
	for(i=0;i<MAX_REGISTER;i++){
		printf(" %s: %d\n", register_str[i], registers[i]); 
	}
	printf(" Program Counter: 0x%08x\n", pc);
	return(0);
}

int add_imi(unsigned int *bytecode, int imi){
	if (imi<-32768 || imi>32767) return (-1);
	*bytecode|= (0xFFFF & imi);
	return(0);
}

int add_sht(unsigned int *bytecode, int sht){
	if (sht<0 || sht>31) return(-1);
	*bytecode|= (0x1F & sht) << 6;
	return(0);
}

int add_reg(unsigned int *bytecode, char *reg, int pos){
	int i;
	for(i=0;i<MAX_REGISTER;i++){
		if(!strcmp(reg,register_str[i])){
			*bytecode |= (i << pos); 
			//printf("add_reg used here \n");
			return(0);
		}
		

	}
	return(-1);
} 

int add_lbl(unsigned int offset, unsigned int *bytecode, char *label){
	char l[MAX_ARG_LEN+1];	
	int j=0;
	while(j<prog_len){
		memset(l,0,MAX_ARG_LEN+1);
		sscanf(&prog[j][0],"%" XSTR(MAX_ARG_LEN) "[^:]:", l);
		if (label!=NULL && !strcmp(l, label)) return(add_imi( bytecode, j-(offset+1)) );
		j++;
	}
	return (-1);
}

int opcode_nop(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0;
	return (0);
}

int opcode_add(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x20; 				// op,shamt,funct
	if (add_reg(bytecode,arg1,11)<0) return (-1); 	// destination register
	if (add_reg(bytecode,arg2,21)<0) return (-1);	// source1 register
	if (add_reg(bytecode,arg3,16)<0) return (-1);	// source2 register
	return (0);
}

int opcode_addi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x20000000; 				// op
	if (add_reg(bytecode,arg1,16)<0) return (-1);	// destination register
	if (add_reg(bytecode,arg2,21)<0) return (-1);	// source1 register
	if (add_imi(bytecode,atoi(arg3))) return (-1);	// constant
	return (0);
}

int opcode_andi(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x30000000; 				// op
	if (add_reg(bytecode,arg1,16)<0) return (-1); 	// destination register
	if (add_reg(bytecode,arg2,21)<0) return (-1);	// source1 register
	if (add_imi(bytecode,atoi(arg3))) return (-1);	// constant 
	return (0);
}

int opcode_beq(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x10000000; 				// op
	if (add_reg(bytecode,arg1,21)<0) return (-1);	// register1
	if (add_reg(bytecode,arg2,16)<0) return (-1);	// register2
	if (add_lbl(offset,bytecode,arg3)) return (-1); // jump 
	return (0);
}

int opcode_bne(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x14000000; 				// op
	if (add_reg(bytecode,arg1,21)<0) return (-1); 	// register1
	if (add_reg(bytecode,arg2,16)<0) return (-1);	// register2
	if (add_lbl(offset,bytecode,arg3)) return (-1); // jump 
	return (0);
}

int opcode_srl(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0x2; 					// op
	if (add_reg(bytecode,arg1,11)<0) return (-1);   // destination register
	if (add_reg(bytecode,arg2,16)<0) return (-1);   // source1 register
	if (add_sht(bytecode,atoi(arg3))<0) return (-1);// shift 
	return(0);
}

int opcode_sll(unsigned int offset, unsigned int *bytecode, char *opcode, char *arg1, char *arg2, char *arg3 ){
	*bytecode=0; 					// op
	if (add_reg(bytecode,arg1,11)<0) return (-1);	// destination register
	if (add_reg(bytecode,arg2,16)<0) return (-1); 	// source1 register
	if (add_sht(bytecode,atoi(arg3))<0) return (-1);// shift 
	return(0);
}

const char *opcode_str[] = {"nop", "add", "addi", "andi", "beq", "bne", "srl", "sll"};
opcode_function opcode_func[] = {&opcode_nop, &opcode_add, &opcode_addi, &opcode_andi, &opcode_beq, &opcode_bne, &opcode_srl, &opcode_sll};

int make_bytecode(){
	unsigned int bytecode;
       	int j=0;
       	int i=0;

	char label[MAX_ARG_LEN+1];
	char opcode[MAX_ARG_LEN+1];
        char arg1[MAX_ARG_LEN+1];
        char arg2[MAX_ARG_LEN+1];
        char arg3[MAX_ARG_LEN+1];

       	printf("ASSEMBLING PROGRAM ...\n");
	while(j<prog_len){
		memset(label,0,sizeof(label)); 
		memset(opcode,0,sizeof(opcode)); 
		memset(arg1,0,sizeof(arg1)); 
		memset(arg2,0,sizeof(arg2)); 
		memset(arg3,0,sizeof(arg3));	

		bytecode=0;

		if (strchr(&prog[j][0], ':')){ //check if the line contains a label
			if (sscanf(&prog[j][0],"%" XSTR(MAX_ARG_LEN) "[^:]: %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) 
				"s %" XSTR(MAX_ARG_LEN) "s", label, opcode, arg1, arg2, arg3) < 2){ //parse the line with label
					printf("ERROR: parsing line %d\n", j);
					return(-1);
			}
		}
		else {
			if (sscanf(&prog[j][0],"%" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) "s %" XSTR(MAX_ARG_LEN) 
				"s %" XSTR(MAX_ARG_LEN) "s", opcode, arg1, arg2, arg3) < 1){ //parse the line without label
					printf("ERROR: parsing line %d\n", j);
					return(-1);
			}
		}
       		
		for (i=0; i<MAX_OPCODE; i++){
                	if (!strcmp(opcode, opcode_str[i]) && ((*opcode_func[i])!=NULL)) {
                               	if ((*opcode_func[i])(j,&bytecode,opcode,arg1,arg2,arg3)<0) {
									printf("ERROR: line %d opcode error (assembly: %s %s %s %s)\n", j, opcode, arg1, arg2, arg3);
								return(-1);
								}
				else {
					printf("0x%08x 0x%08x\n",ADDR_TEXT + 4*j, bytecode);
					text[j] = bytecode; //Bytecode stored in text
					break;
				}
                	}
			if(i==(MAX_OPCODE-1)) {
				printf("ERROR: line %d unknown opcode\n", j);
				return(-1);
			}
        	}
		j++;
       	}
       	printf("... DONE!\n");
       	
       	return(0);
}

int exec_bytecode(){
		
		
		
        printf("EXECUTING PROGRAM s  ...\n");

        pc = ADDR_TEXT; //set program counter to the start of our program
        
        int k = 1;
        int mask = 0x00;
        
        
		for (int i=0; i<=prog_len;i++){
			//ADDI
			if ((text[i] >> 26) == 8 ){
				printf("ADDI\n");
				//source s
				mask = 0x03e00000;
				int source_s = text[i] & mask;
				source_s = source_s >> 21;
				printf("ADDI Source c = %s\n", register_str[source_s]);
				
				//Immediate 
				mask = 0x0000ffff;
				int imm = text[i] & mask;
				printf("ADDI immediate = %d\n", imm);
				
				//ADDI destination
				mask = 0x001f0000;
				int addi_dest = text[i] & mask;
				addi_dest = addi_dest >> 16;
				printf("ADDI destination = %s\n", register_str[addi_dest]);
				
				//ADDI to registers
				registers[addi_dest] = registers[source_s] + imm;
				
			}
			//Add instruction
			else if ((text[i] >> 26 ==0) && (text[i] << 21 == 67108864)){
				printf("ADD\n");
				//Source s
				mask = 0x03e00000;
				int source_s = text[i] & mask;
				source_s = source_s >> 21;
				printf("Add Source s = %d\n", source_s);
				
				//Source t
				mask = 0x001f0000;
				int source_t = text[i] & mask;
				source_t = source_t >> 16;
				printf("Add Source t = %d\n", source_t);
				
				//Destination
				mask = 0x0000d800;
				int add_dest = text[i] & mask;
				add_dest = add_dest >> 11;
				printf("Add destination = %s\n", register_str[add_dest]);
				
				//Add to registers
				registers[add_dest] = registers[source_s]+registers[source_t];
				
				}
			else if	(text[i] >> 26 == 12){
				printf("ANDI\n");
				//Source s
				mask = 0x03e00000;
				int source_s = text[i] & mask;
				source_s = source_s >> 21;
				printf("ANDI source s = %d\n", source_s);
				
				//ANDI immediate
				mask = 0x0000ffff;
				int imm = text[i] & mask;
				printf("ANDI immediate = %d\n", imm);
				
				//ANDI destination
				mask = 0x001f0000;
				int andi_dest = text[i] & mask;
				andi_dest = andi_dest >> 16;
				printf("ANDI destination = %s\n", register_str[andi_dest]);
				
				//ANDI to registers
				registers[andi_dest] = registers[source_s] + imm;
				
				}
			else if ((text[i] >> 26 == 0) && (text[i] << 30) == k << 31){
				printf("SRL\n");
				
				
				}
			else if ((text[i] >> 26 == 0) && (text[i] << 26 == 0)){
				printf("SLL\n");
				}
			else if ((text[i] >> 26 == 4)){
				printf("BEQ\n");
				}
			else if (text[i] >> 26 == 5){
				printf("BNE\n");
				}	
			
		}	
		printf("Bytecode read from file\n");
		
        //printf("... needs implementing!\n");
        //printf("0x%08x\n", *bytecode);
        
        
		
		printf("----------------------");
		printf("\n");
		printf("----------------------");
		printf("\n");
		printf("----------------------");
		printf("\n");
				
		
		
			
		printf("----------------------");
		printf("\n");
		printf("----------------------");
		printf("\n");
		printf("----------------------");
		printf("\n");
				
		

        //print_registers(); // print out the state of registers at the end of execution
		print_registers();
		
		 printf("This is text[1] %d\n", text[1]);
		 int new = text[1] >> 1;
		 printf("Shifted right: %d\n", new);
		 int change = text[1] - new;
		 printf("Difference: %d\n", change);

        printf("... DONE!\n");
        return(0);

}


int load_program(){
       int j=0;
       FILE *f;
       

       printf("LOADING PROGRAM ...\n");

       f = fopen ("prog1.txt", "r");

       if (f==NULL) {
       		printf("ERROR: program not found\n");
       }

       while(fgets(&prog[prog_len][0], MAX_LINE_LEN, f) != NULL) {
               	prog_len++;
       }

       printf("PROGRAM:\n");
       for (j=0;j<prog_len;j++){
               	printf("%d: %s",j, &prog[j][0]);
       }
       
      

       printf("... DONE!\n");

       return(0);
}


int main(){
	if (load_program()<0) 	return(-1);        
	if (make_bytecode()<0) 	return(-1); 
	if (exec_bytecode()<0) 	return(-1); 
   	return(0);
}

