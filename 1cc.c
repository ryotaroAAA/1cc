#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<stdarg.h>

enum {
	TK_NUM =256,
	TK_EOF,
};

typedef struct {
	int ty;
	int val;
	char *input;
} Token;

typedef struct Node{
	int ty;
	struct Node *lhs;
	struct Node *rhs;
	int val;
} Node;

enum {
	ND_NUM = 256,
};

Node *expr();
Node *mul();
Node *term();

char *user_input;
int pos = 0;

Token tokens[100];

void error(char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

void error_at(char *loc, char *msg){
	int posi = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s\n", posi, "");
	fprintf(stderr, "^ %s\n", msg);
	exit(1);
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
	Node *node = malloc(sizeof(Node));
	node->ty = ty;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = malloc(sizeof(Node));
	node->ty = ND_NUM;
	node->val = val;
	return node;
}

int consume(int ty) {
	if (tokens[pos].ty != ty)
		return 0;
	pos++;
	return 1;
}

Node *expr(){
	Node *node = mul();

	for (;;) {
		if (consume('+'))
			node = new_node('+', node, mul());
		else if (consume('-'))
			node = new_node('-', node, mul());
		else 
			return node;
	}
}

Node *mul(){
	Node *node = term();

	for (;;) {
		if (consume('*'))
			node = new_node('*', node, term());
		else if (consume('/'))
			node = new_node('/', node, term());
		else 
			return node;
	}
}

Node *term() {
	if (consume('(')) {
		Node *node = expr();
		if (!consume(')'))
			error_at(tokens[pos].input, "開きカッコに対応する閉じカッコがありません");
		return node;
	}

	if (tokens[pos].ty == TK_NUM)
		return new_node_num(tokens[pos++].val);

	error_at(tokens[pos].input,
		"数値でも開きカッコでもないトークンです");
}

void tokenize() {
	char *p = user_input;

	int i = 0;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p=='+'||*p=='-') {
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}

		if (isdigit(*p)) {
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}

		error_at(p, "tokenize error");
	}
	tokens[i].ty = TK_EOF;
	tokens[i].input = p;
}

void gen(Node *node){
	if (node->ty == ND_NUM) {
		printf("	push %d\n", node->val);
		return;
	}
	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch (node->ty) {
		case '+':
			printf("	add rax, rdi\n");
			break;
		case '-':
			printf("	sub rax, rdi\n");
		case '*':
			printf("	imul rdi\n");
			break;
		case '/':
			printf("	cqo\n");
			printf("	idiv rdi\n");
			break;
	}

	printf("	push rax\n");
}

int main (int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}

	user_input = argv[1];
	tokenize();
	Node *node = expr();
	
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	gen(node);

	printf("	pop rax\n");
	printf("	ret\n");
	return 0;
}
