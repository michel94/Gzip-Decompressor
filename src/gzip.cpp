/*Authors: Rui Pedro Paiva, Miguel Martins Duarte, Guilherme Simões Santos Graça
Teoria da Informação, LEI, 2013/2014*/

#include <unistd.h>
#include <cstdlib>

#include "gzip.h"
#include "huffman.h"
#include <string.h>

int availBits = 0;
FILE *gzFile;  //ponteiro para o ficheiro a abrir
unsigned int rb = 0;  //último byte lido (poderá ter mais que 8 bits, se tiverem sobrado alguns de leituras anteriores)


int mask(int size){
/*
	This function returns a mask given its size;
*/
	unsigned int m = 0;
	for(int i=0; i<size; i++){
		m = m << 1;
		m = m | 0x01;
	}
	return m;

}

int getBits(int needBits, int* availBits, unsigned int* rb, FILE* gzFile){
/*
	This function is used to read bits from the file.
	It works by reading a byte when it is needed and
	applying a mask to retrieve the ones needed.
*/
	unsigned char byte;
	if(needBits <= 0)
		return 0;

	while (*availBits < needBits) //por ciclo while
	{
		fread(&byte, 1, 1, gzFile);
		*rb = (byte << *availBits) | *rb;
		*availBits += 8;
	}
	unsigned int r = *rb & mask(needBits);
	*rb = *rb >> needBits;
	*availBits -= needBits;

	return r;
}

void printBin(unsigned int num, int size){
/*
	This auxiliary function prints a string given a
	binary huffman code as an integer.
*/

	int i=0;
	unsigned int *bin = (unsigned int*)malloc(size * sizeof(int));
	memset(bin, 0, size * sizeof(int));
	while(num){
		bin[size - 1 - i] = num % 2;
		num /= 2;
		i++;
	}
	for(i=0; i<size; i++){
		printf("%d", bin[i]);
	}
	putchar('\n');
}

void getString(char str[], unsigned int num, int size){
/*
	This auxiliary function creates a string from a 
	binary huffman code, given as an integer.
*/

	int i=0;
	unsigned int *bin = (unsigned int*)malloc((size+1) * sizeof(int));
	memset(bin, 0, (size+1) * sizeof(char));
	
  while(num){
		bin[size - 1 - i] = num % 2;
		num /= 2;
		i++;
	}
  str[0] = 0;
	for(i=0; i<size; i++){
		char c[2];
		sprintf(c, "%d", bin[i]);
		strcat(str, c);
	}
  str[size] = 0;

}




void length2intcode(unsigned int *code_length, unsigned int* codes, int max_length, int n_codes){
/*
	This function is responsible for getting the huffman codes
	given their lengths.
*/
	int i, s;
	unsigned int cur = 0;
	for(s=1; s<=max_length; s++){
		for(i=0; i<n_codes; i++){
			if(code_length[i] == s){
				codes[i] = cur;
				cur++;
			}
		}
		cur = cur << 1;
	}
}

void read_using_hufftree(unsigned int* dest, HuffmanTree* hufTree, int size){
/*
	This function is responsible for reading from the file,
	bit by bit, using a previously read huffman tree.
*/

	int v, cur, i, c;
	for(i=0; i<size;){
		while(1){
    	unsigned int bit = getBits(1, &availBits, &rb, gzFile);
			v = v << 1 + bit;
    		cur = nextNode(hufTree, '0' + bit);
	        //printf("cur: %d\n", cur);
	        if(cur >= -1){
	        	int bits = 0;
	        	if(cur == -1){
	        		dest[i] = 0;
	        		i++;
	        	}else{
	        		if(cur < 16){
	        			dest[i] = cur;
	        			i++;
	        		}
		        	if(cur == 16){
		        		int copy = getBits(2, &availBits, &rb, gzFile) + 3;
		        		for(c=i; c < i + copy; c++){	
		        			dest[c] = dest[i-1];
		        		}
		        		i+=copy;
		        	}else if(cur == 17){
		        		int copy = getBits(3, &availBits, &rb, gzFile) + 3;
		        		for(c=i; c < i + copy; c++){	
		        			dest[c] = 0;
		        		}
		        		i+=copy;
		        	}else if(cur == 18){
		        		int copy = getBits(7, &availBits, &rb, gzFile) + 11;
		        		for(c=i; c < i + copy; c++){	
		        			dest[c] = 0;
		        		}
		        		i+=copy;
		        	}
	        	}
	          	hufTree -> curNode = hufTree -> root;
	          	break;
	        }
		}
  
	}
}

void intcodes2tree(unsigned int* intcodes, unsigned int* compCod, HuffmanTree* hufTree, int size){
/*
	This function is responsible for initializing the 
	huffman tree with the respective codes.
*/
	int i;
	for(i=0; i<size; i++){
		if(compCod[i] > 0){
			char str[25];
			getString(str, intcodes[i], compCod[i]);
			addNode(hufTree, str, i, 0);
		}
	}
}

void writelz77(char* output, int* pos, int length, int dist, int printing){
/*
	This function is responsible for writing characters 
	already read in the output string, similarly to the lz77
	algorithm.
*/
	int i;
	for(i=0; i<length; i++){
		output[*pos+i] = output[*pos-dist+i];
		if(printing){
			printf("%c", output[*pos+i]);
			fflush(stdout);
		}
	}
	*pos += length;
}


//função principal, a qual gere todo o processo de descompactação
int main(int argc, char** argv)
{
	//--- Gzip file management variables
	
	long fileSize;
	long origFileSize;
	int numBlocks = 0;	
	gzipHeader gzh;
	unsigned char byte;  //variável temporária para armazenar um byte lido directamente do ficheiro	
	char needBits = 0;


	//--- obter ficheiro a descompactar
	//char fileName[] = "FAQ.txt.gz";

	if (argc != 2)
	{
		printf("Linha de comando inválida!!!");
		return -1;
	}
	char * fileName = argv[1];

	//--- processar ficheiro
	gzFile = fopen(fileName, "r");
	fseek(gzFile, 0L, SEEK_END);
	fileSize = ftell(gzFile);
	fseek(gzFile, 0L, SEEK_SET);

	//ler tamanho do ficheiro original (acrescentar: e definir Vector com símbolos
	origFileSize = getOrigFileSize(gzFile);


	//--- ler cabeçalho
	int erro = getHeader(gzFile, &gzh);
	if (erro != 0)
	{
		printf ("Formato inválido!!!");
		return -1;
	}

	//--- Para todos os blocos encontrados
	unsigned char BFINAL;
	FILE* f = fopen(gzh.fName, "w");
	
	char* output = (char *)malloc(origFileSize*sizeof(char));
	int op = 0;


	do
	{
		//--- ler o block header: primeiro byte depois do cabeçalho do ficheiro
		needBits = 3;
		if (availBits < needBits)
		{
			fread(&byte, 1, 1, gzFile);
			rb = (byte << availBits) | rb;
			availBits += 8;
		}
		
		//obter BFINAL
		//ver se é o último bloco
		BFINAL = rb & 0x01; //primeiro bit é o menos significativo
		printf("BFINAL = %d\n", BFINAL);
		rb = rb >> 1; //descartar o bit correspondente ao BFINAL
		availBits -=1;
		

		//analisar block header e ver se é huffman dinâmico					
		if (!isDynamicHuffman(rb)){  //ignorar bloco se não for Huffman dinâmico
			continue;
		}

		//--- Se chegou aqui --> compactado com Huffman dinâmico --> descompactar

		rb = rb >> 2; //descartar os 2 bits correspondentes ao BTYPE
		availBits -= 2;
		
		unsigned char byte1, byte2;
		int HLIT, HDIST, HCLEN;
		int i, s, cur=0;


		// reading values of HLIT, HDIST and HCLEN
		HLIT = getBits(5, &availBits, &rb, gzFile);
		HDIST = getBits(5, &availBits, &rb, gzFile);
		HCLEN = getBits(4, &availBits, &rb, gzFile);

		printf("Constants needed: HLIT:%d HDIST:%d HCLEN:%d\n", HLIT, HDIST, HCLEN);

		// reading first code lengths that will be used to generate the first huffman tree.
		int loc[] = {16,17,18,0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
		unsigned int comp_cod[18];
		memset(comp_cod, 0, sizeof(comp_cod));
		for(i=0; i<HCLEN+4; i++){
			int v = getBits(3, &availBits, &rb, gzFile);
			comp_cod[loc[i]] = v;
		}

		printf("Length of the codes used to create the first huffman tree:\n");
		for(i=0; i<19; i++){
			printf("%d\n", comp_cod[i]);
		}

		//getting huffman codes from code lengths
		unsigned int codes[19];
		length2intcode(comp_cod, codes, 7, 19);

		printf("Codes obtained from the lengths:\n");
		for(i=0; i<19; i++){
			printf("%d %d %d ", i, comp_cod[i], codes[i]);
			printBin(codes[i], comp_cod[i]);
		}
		
		//creating huffman tree
		HuffmanTree* hufTree = createHFTree();
		intcodes2tree(codes, comp_cod, hufTree, 19);
	  char str[100];
	  printf("Tree of the first huffman codes, used to read literal/length and distance codes lengths:\n");
	  printTree(hufTree->root, str, 0); // printing tree for debugging


	  //reading lengths of the literal/length codes
		unsigned int hlits[286];
		memset(hlits, 0, sizeof(hlits));
		read_using_hufftree(hlits, hufTree, HLIT + 257);


		//reading lengths of the distance codes
		unsigned int hdists[30];
		memset(hdists, 0, sizeof(hdists));
		read_using_hufftree(hdists, hufTree, HDIST+1);


		// getting distance codes from their length and initialization of the respective huffman tree.
		unsigned int distscodes[30];
		length2intcode(hdists, distscodes, 15, 30);
		HuffmanTree* distsTree = createHFTree();
		intcodes2tree(distscodes, hdists, distsTree, 30);
		printf("Tree of the Distance Codes:\n");
		printTree(distsTree->root, str, 0); //printing the tree for debug
		

		// getting literal/length codes from their length and initialization of the respective huffman tree.
		unsigned int litcodes[300];
		length2intcode(hlits, litcodes, 16, HLIT + 257);
		HuffmanTree* dataTree = createHFTree();
		intcodes2tree(litcodes, hlits, dataTree, 285);
		printf("Tree of the Literals/Length Codes:\n");
		printTree(dataTree->root, str, 0); //printing the tree for debug

		
		int extra_length_hc[] = {
			11, 13, 15, 17, 19, 23, 27,
			31, 35, 43, 51, 59, 67, 83,
			99, 115, 131, 163, 195, 227
		};
		int extra_dist_hc[] = {
			4, 6, 8, 12, 16, 24, 32, 48,
			64, 96, 128, 192, 256, 384,
			512, 768, 1024, 1536, 2048,
			3072, 4096, 6144, 8192,
			12288, 16384, 24576
		};

		int readBit;
		int debug = 0;
		int printing = 0;
		while(1){
			readBit = getBits(1, &availBits, &rb, gzFile);
			cur = nextNode(dataTree, '0' + readBit);
			if(cur >= -1){
				if(cur == 256)
					break;
				else if(cur < 256){
					if(printing){
						printf("%c", cur);
						fflush(stdout);
					}
					output[op++] = cur;
				}else{
					int length, distance;
					if(cur == 285)
						length = 258;
					else{						
						if(cur < 265){
							length = cur - 254;
						}else{
							int extra = (cur - 261) / 4;
							
							int bits = getBits(extra, &availBits, &rb, gzFile);
							length = bits + extra_length_hc[cur - 265];
						}
					}
					if(debug) printf("length: %d ", length);
					
					int readBitDist, curDist = 0;
					while(1){
						readBitDist = getBits(1, &availBits, &rb, gzFile);
						curDist = nextNode(distsTree, '0' + readBitDist);
						if(curDist >= -1){
							if(curDist == -1){
								printf("\nError, code doesn't exist\n");
							}else{

								distance = curDist + 1;
								if(curDist > 3){
									int extra_dist = getBits( (curDist-2)/2, &availBits, &rb, gzFile);
									distance = extra_dist + extra_dist_hc[curDist - 4] + 1;
								}
								if(debug) printf(" distance: %d\n", distance);
								writelz77(output, &op, length, distance, printing);
								
							}
							break;
						}

					}
					
				}
				resetCurNode(dataTree);
				resetCurNode(distsTree);

			}


		}
		
		//actualizar número de blocos analisados
		numBlocks++;
		printf("This was the output for block %d\n", numBlocks);
		
	}while(BFINAL == 0);
	output[op] = '\0';
	fprintf(f, output);
	free(output);
	


	//terminações			
	fclose(gzFile);
	printf("End: %d bloco(s) analisado(s).\n", numBlocks);


    //RETIRAR antes de criar o executável final
    //system("PAUSE");
    return EXIT_SUCCESS;
}


//---------------------------------------------------------------
//Lê o cabeçalho do ficheiro gzip: devolve erro (-1) se o formato for inválidodevolve, ou 0 se ok
int getHeader(FILE *gzFile, gzipHeader *gzh) //obtém cabeçalho
{
	unsigned char byte;

	//Identicação 1 e 2: valores fixos
	fread(&byte, 1, 1, gzFile);
	(*gzh).ID1 = byte;
	if ((*gzh).ID1 != 0x1f) return -1; //erro no cabeçalho
	
	fread(&byte, 1, 1, gzFile);
	(*gzh).ID2 = byte;
	if ((*gzh).ID2 != 0x8b) return -1; //erro no cabeçalho
	
	//Método de compressão (deve ser 8 para denotar o deflate)
	fread(&byte, 1, 1, gzFile);
	(*gzh).CM = byte;
	if ((*gzh).CM != 0x08) return -1; //erro no cabeçalho
				
	//Flags
	fread(&byte, 1, 1, gzFile);
	unsigned char FLG = byte;
	
	//MTIME
	char lenMTIME = 4;	
	fread(&byte, 1, 1, gzFile);
	(*gzh).MTIME = byte;
	for (int i = 1; i <= lenMTIME - 1; i++)
	{
		fread(&byte, 1, 1, gzFile);
		(*gzh).MTIME = (byte << 8) + (*gzh).MTIME;				
	}
					
	//XFL (not processed...)
	fread(&byte, 1, 1, gzFile);
	(*gzh).XFL = byte;
	
	//OS (not processed...)
	fread(&byte, 1, 1, gzFile);
	(*gzh).OS = byte;
	
	//--- Check Flags
	(*gzh).FLG_FTEXT = (char)(FLG & 0x01);
	(*gzh).FLG_FHCRC = (char)((FLG & 0x02) >> 1);
	(*gzh).FLG_FEXTRA = (char)((FLG & 0x04) >> 2);
	(*gzh).FLG_FNAME = (char)((FLG & 0x08) >> 3);
	(*gzh).FLG_FCOMMENT = (char)((FLG & 0x10) >> 4);
				
	//FLG_EXTRA
	if ((*gzh).FLG_FEXTRA == 1)
	{
		//ler 2 bytes XLEN + XLEN bytes de extra field
		//1º byte: LSB, 2º: MSB
		char lenXLEN = 2;

		fread(&byte, 1, 1, gzFile);
		(*gzh).xlen = byte;
		fread(&byte, 1, 1, gzFile);
		(*gzh).xlen = (byte << 8) + (*gzh).xlen;
		
		(*gzh).extraField = new unsigned char[(*gzh).xlen];
		
		//ler extra field (deixado como está, i.e., não processado...)
		for (int i = 0; i <= (*gzh).xlen - 1; i++)
		{
			fread(&byte, 1, 1, gzFile);
			(*gzh).extraField[i] = byte;
		}
	}
	else
	{
		(*gzh).xlen = 0;
		(*gzh).extraField = 0;
	}
	
	//FLG_FNAME: ler nome original	
	if ((*gzh).FLG_FNAME == 1)
	{			
		(*gzh).fName = new char[1024];
		unsigned int i = 0;
		do
		{
			fread(&byte, 1, 1, gzFile);
			if (i <= 1023)  //guarda no máximo 1024 caracteres no array
				(*gzh).fName[i] = byte;
			i++;
		}while(byte != 0);
		if (i > 1023)
			(*gzh).fName[1023] = 0;  //apesar de nome incompleto, garantir que o array termina em 0
	}
	else
		(*gzh).fName = 0;
	
	//FLG_FCOMMENT: ler comentário
	if ((*gzh).FLG_FCOMMENT == 1)
	{			
		(*gzh).fComment = new char[1024];
		unsigned int i = 0;
		do
		{
			fread(&byte, 1, 1, gzFile);
			if (i <= 1023)  //guarda no máximo 1024 caracteres no array
				(*gzh).fComment[i] = byte;
			i++;
		}while(byte != 0);
		if (i > 1023)
			(*gzh).fComment[1023] = 0;  //apesar de comentário incompleto, garantir que o array termina em 0
	}
	else
		(*gzh).fComment = 0;

	
	//FLG_FHCRC (not processed...)
	if ((*gzh).FLG_FHCRC == 1)
	{			
		(*gzh).HCRC = new unsigned char[2];
		fread(&byte, 1, 1, gzFile);
		(*gzh).HCRC[0] = byte;
		fread(&byte, 1, 1, gzFile);
		(*gzh).HCRC[1] = byte;		
	}
	else
		(*gzh).HCRC = 0;
	
	return 0;
}


//---------------------------------------------------------------
//Analisa block header e vê se é huffman dinâmico
int isDynamicHuffman(unsigned char rb)
{				
	unsigned char BTYPE = rb & 0x03;
					
	if (BTYPE == 0) //--> sem compressão
	{
		printf("Ignorando bloco: sem compactação!!!\n");
		return 0;
	}
	else if (BTYPE == 1)
	{
		printf("Ignorando bloco: compactado com Huffman fixo!!!\n");
		return 0;
	}
	else if (BTYPE == 3)
	{
		printf("Ignorando bloco: BTYPE = reservado!!!\n");
		return 0;
	}
	else
		return 1;		
}


//---------------------------------------------------------------
//Obtém tamanho do ficheiro original
long getOrigFileSize(FILE * gzFile)
{
	//salvaguarda posição actual do ficheiro
	long fp = ftell(gzFile);
	
	//últimos 4 bytes = ISIZE;
	fseek(gzFile, -4, SEEK_END);
	
	//determina ISIZE (só correcto se cabe em 32 bits)
	unsigned long sz = 0;
	unsigned char byte;
	fread(&byte, 1, 1, gzFile);
	sz = byte;
	for (int i = 0; i <= 2; i++)
	{
		fread(&byte, 1, 1, gzFile);
		sz = (byte << 8*(i+1)) + sz;
	}

	
	//restaura file pointer
	fseek(gzFile, fp, SEEK_SET);
	
	return sz;		
}


//---------------------------------------------------------------
void bits2String(char *strBits, unsigned char byte)
{
	char mask = 0x01;  //get LSbit
	
	strBits[8] = 0;
	for (char bit, i = 7; i >= 0; i--)
	{		
		bit = byte & mask;
		strBits[i] = bit +48; //converter valor numérico para o caracter alfanumérico correspondente
		byte = byte >> 1;
	}	
}
