/*Author: Rui Pedro Paiva
Teoria da Informação, LEI, 2007/2008*/

#include "huffman.h"


HuffmanTree* createHFTree()
{
	HuffmanTree *hft = new HuffmanTree;

	hft->root = createHFNode(-1, 0, NULL, NULL);
	hft->curNode = hft->root;

	return hft;
}


HFNode* createHFNode(short index, short level, HFNode *left, HFNode *right)
{
	HFNode *hfn = new HFNode;

	hfn->index = index;  //nó vazio
	hfn->level = level; //nível 0 na árvore
	hfn->left = left;  //não tem filhos
	hfn->right = right;

	return hfn;
}

//adiciona nó à árvore 
//recebe a árvore onde inserir, a string com o código (representado por string com 0s e 1s), o índice no alfabeto e o campo verbose
//devolve 0 se adicionou bem; -1 se o nó já existe; -2 se o código deixar de ser de prefixo
int addNode(HuffmanTree *hft, char *s, int ind, short verbose)
{
	HFNode *tmp = hft->root;  //nó para atravessar a árvore

	int lv = 0, len = (int)strlen(s), index;
	int found = 0;
	char direction;
	int pos = -3;
				
	while(lv < len && !found)
	{			
		if (tmp->index != -1)  //tentando criar filho de folha --> deixaria de ser código de prefixo...
		{
			pos = -2;
			found = 1;
		}
		else
		{			
			direction = s[lv];
			
			if (direction == '0')
			{
				if (lv != len-1 && tmp->left != NULL)  //continua descida
				{					
					tmp = tmp->left;
				}
				else if (tmp->left != NULL) // nó já inserido
				{
					pos = -1;
					found = 1;
				}
				else //cria nó à esquerda
				{
					if (lv == len-1)  //folha						
						index = ind;
					else
						index = -1;

					HFNode *hf = createHFNode(index, lv+1, NULL, NULL);
					tmp->left = hf;
					tmp = tmp->left;
				}
			}
			else if (direction == '1')
			{				
				if (lv != len -1 && tmp->right != NULL)
				{					
					tmp = tmp->right;
				}
				else if (tmp->right != NULL) // nó já inserido
				{
					  pos = -1;
					  found = 1;
				}
				else //cria nó à direita
				{
					if (lv == len-1)  //folha
						index = ind;
					else
						index = -1;

					HFNode *hf = createHFNode(index, lv+1, NULL, NULL);
					tmp->right = hf;
					tmp = tmp->right;
				}			
			}
		}				
		lv++;				
	}
	
			
	if (!found)
		pos = tmp->index;
		
	if (verbose)
	{
		if (pos == -1)
			printf("Código '%s' já inserido!!!\n", s);
		else if (pos == -2)
			printf("Código '%s': tentando extender folha!!!\n", s);
		else
			printf("Código '%s' inserido com sucesso!!!\n", s);
	}
	
	return pos;
}				

void printTree(HFNode *tree, char str[], int tam){
/*
	This function prints the huffman tree using a dfs algoritm.
*/
	if(tree == NULL) return;
	if(isLeaf(tree))
		printf("Tree Node: %s - %d\n", str, tree->index);
	str[tam] = '0';
	str[tam + 1] = '\0';
	printTree(tree -> left, str, tam + 1);
	str[tam] = '1';
	str[tam + 1] = '\0';
	printTree(tree->right, str, tam + 1);
}

//actualiza nó corrente na árvore com base no nó actual e nó próximo bit
//utilizada para pesquisa na árvore bit a bit (e não código inteiro de uma vez) --> situação correspodente ao deflate
//devolve index se é folha; -1 se não encontrou o nó; -2 se encontrou mas ainda não é folha
int nextNode(HuffmanTree *hft, char c)
{
	int pos;
	
	if (isLeaf(hft->curNode))
		pos = -1;
	
	if (c == '0')
		if (hft->curNode->left != NULL)
		{
			hft->curNode = hft->curNode->left;
			if (isLeaf(hft->curNode))
				pos = hft->curNode->index;
			else
				pos = -2;
		}
		else
			pos = -1;
	else // c == '1'
		if (hft->curNode->right != NULL)
		{
			hft->curNode = hft->curNode->right;
			if (isLeaf(hft->curNode))
				pos = hft->curNode->index;
			else
				pos = -2;
		}
		else
			pos = -1;								
	
	return pos;
}



//procura código na árvore a partir da raíz (representado por String com 0s e 1s): procura código inteiro
int findNode(HuffmanTree *hft, char* s, short verbose)
{
	return findNode(hft, s, hft->root, verbose);
}



//procura código na árvore, a partir do nó actual (representado por String com 0s e 1s)
//devolve índice no alfabeto, se encontrou; -1 se não encontrou; -2 se é prefixo de código existente; 
int findNode(HuffmanTree *hft, char *s, HFNode *cur, short verbose)
{
	HFNode *tmp = cur;
	int lv = 0, len = (int)strlen(s);
	int found = 1;
	int pos;
	
	while(lv < len && found)
	{			
		char direction = s[lv];
		
		if (direction == '0')
		{
			if (tmp->left != NULL)
				tmp = tmp->left;
			else
				found = 0;
		}
		else if (direction == '1')
		{
			if (tmp->right != NULL)
				tmp = tmp->right;
			else
				found = 0;
		}
		
		lv++;
	}
			
	if (!found)
		pos = -1;
	else if(tmp->index == -1)
		pos = -2;
	else
		pos = tmp->index;
		
	if (verbose)
	{
		if (pos == -1)
			printf("Código '%s' não encontrado!!!\n", s);
		else if (pos == -2)
			printf("Código '%s' não encontrado, mas prefixo!!!\n", s);
		else
			printf("Código '%s' corresponde à posição %d do alfabeto\n", s, pos);
	}
					
	return pos;
}	



//verifica se o nó é folha
short isLeaf(HFNode *n)
{
	if (n->left == NULL && n->right == NULL)
		return 1;
	else
		return 0;
}	


//reposicionar nó corrente na raíz
void resetCurNode (HuffmanTree *hft)
{
	hft->curNode = hft->root;
}
