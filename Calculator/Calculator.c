#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

static jmp_buf ebuf;
static char buffer[40];

typedef enum ASTNodeType
{
	Undefined,
	OperatorPlus,
	OperatorMinus,
	OperatorMul,
	OperatorDiv,
	UnaryMinus,
	NumberValue
}ASTNodeType;
typedef struct ASTNode
{
	ASTNodeType Type;
	double      Value;
	struct ASTNode* Left;
	struct ASTNode* Right;

}ASTNode;

ASTNode* CreateNode(ASTNodeType type, ASTNode* left, ASTNode* right)
{
	ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
	if (!node)
	{
		snprintf(buffer, 40, "Error allocating memory");
		longjmp(ebuf, 1);
	}
	node->Type = type;
	node->Left = left;
	node->Right = right;

	return node;
}
ASTNode* CreateUnaryNode(ASTNode* left)
{
	ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
	if (!node)
	{
		snprintf(buffer, 40, "Error allocating memory");
		longjmp(ebuf, 1);
	}
	node->Type = UnaryMinus;
	node->Left = left;
	node->Right = NULL;

	return node;
}
ASTNode* CreateNodeNumber(double value)
{
	ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
	if (!node)
	{
		snprintf(buffer, 40, "Error allocating memory");
		longjmp(ebuf, 1);
	}
	node->Type = NumberValue;
	node->Value = value;

	return node;
}

static ASTNode* Expression();
static ASTNode* Expression1();
static ASTNode* Term();
static ASTNode* Term1();
static void GetNextToken();
static ASTNode* Factor();
static void Match(char expected);
static double GetNumber();

typedef enum TokenType
{
	Error,
	Plus,
	Minus,
	Mul,
	Div,
	EndOfText,
	OpenParenthesis,
	ClosedParenthesis,
	Number
}TokenType;

struct Token
{
	TokenType   Type;
	double      Value;
	char      Symbol;
};
struct Parser
{
	struct Token m_crtToken;
	const char* m_Text;
	size_t m_Index;
}Parser;

ASTNode* Expression()
{
	ASTNode* tnode = Term();
	ASTNode* e1node = Expression1();

	return CreateNode(OperatorPlus, tnode, e1node);
}
ASTNode* Expression1()
{
	ASTNode* tnode;
	ASTNode* e1node;
	switch (Parser.m_crtToken.Type)
	{
	case Plus:
		GetNextToken();
		tnode = Term();
		e1node = Expression1();

		return CreateNode(OperatorPlus, e1node, tnode);

	case Minus:
		GetNextToken();
		tnode = Term();
		e1node = Expression1();

		return CreateNode(OperatorMinus, e1node, tnode);
	}
	return CreateNodeNumber(0);
}
ASTNode* Term()
{
	ASTNode* fnode = Factor();
	ASTNode* t1node = Term1();

	return CreateNode(OperatorMul, fnode, t1node);
}
ASTNode* Term1()
{
	ASTNode* fnode;
	ASTNode* t1node;

	switch (Parser.m_crtToken.Type)
	{
	case Mul:
		GetNextToken();
		fnode = Factor();
		t1node = Term1();
		return CreateNode(OperatorMul, t1node, fnode);

	case Div:
		GetNextToken();
		fnode = Factor();
		t1node = Term1();
		return CreateNode(OperatorDiv, t1node, fnode);
	}

	return CreateNodeNumber(1);
}
ASTNode* Factor()
{
	ASTNode* node;
	switch (Parser.m_crtToken.Type)
	{
	case OpenParenthesis:
	{
		GetNextToken();
		node = Expression();
		Match(')');
		return node;
	}
	case Minus:
	{
		GetNextToken();
		node = Factor();
		return CreateUnaryNode(node);
	}
	case Number:
	{
		double value = Parser.m_crtToken.Value;
		GetNextToken();
		return CreateNodeNumber(value);
	}
	default:
	{
		snprintf(buffer, 40, "Unexpected token '%c' at position %d", Parser.m_crtToken.Symbol, Parser.m_Index);
		longjmp(ebuf, 1);
	}
	}
}
ASTNode* Parse(const char* text)
{
	Parser.m_Text = text;
	Parser.m_Index = 0;
	GetNextToken();

	return Expression();
}

void Match(char expected)
{
	if (Parser.m_Text[Parser.m_Index - 1] == expected)
		GetNextToken();
	else
	{
		snprintf(buffer, 40, "Expected token '%c' at position %d", expected, Parser.m_Index);
		longjmp(ebuf, 1);
	}
}
void SkipWhitespaces()
{
	while (isspace(Parser.m_Text[Parser.m_Index])) Parser.m_Index++;
}
void GetNextToken()
{
	// ignore white spaces
	SkipWhitespaces();

	Parser.m_crtToken.Value = 0;
	Parser.m_crtToken.Symbol = 0;

	// test for the end of text
	if (Parser.m_Text[Parser.m_Index] == 0)
	{
		Parser.m_crtToken.Type = EndOfText;
		return;
	}

	// if the current character is a digit read a number
	if (isdigit(Parser.m_Text[Parser.m_Index]))
	{
		Parser.m_crtToken.Type = Number;
		Parser.m_crtToken.Value = GetNumber();
		return;
	}

	Parser.m_crtToken.Type = Error;

	// check if the current character is an operator or parentheses
	switch (Parser.m_Text[Parser.m_Index])
	{
	case '+': Parser.m_crtToken.Type = Plus; break;
	case '-': Parser.m_crtToken.Type = Minus; break;
	case '*': Parser.m_crtToken.Type = Mul; break;
	case '/': Parser.m_crtToken.Type = Div; break;
	case '(': Parser.m_crtToken.Type = OpenParenthesis; break;
	case ')': Parser.m_crtToken.Type = ClosedParenthesis; break;
	}

	if (Parser.m_crtToken.Type != Error)
	{
		Parser.m_crtToken.Symbol = Parser.m_Text[Parser.m_Index];
		Parser.m_Index++;
	}
	else
	{
		snprintf(buffer, 40, "Unexpected token '%c' at position %d", Parser.m_Text[Parser.m_Index], Parser.m_Index);
		longjmp(ebuf, 1);
	}
}
void FreeNode(ASTNode* ast)
{
	if (ast == NULL)
		return;
	FreeNode(ast->Left);
	FreeNode(ast->Right);
	free(ast);
}
double GetNumber()
{
	SkipWhitespaces();

	int index = Parser.m_Index;
	while (isdigit(Parser.m_Text[Parser.m_Index])) Parser.m_Index++;
	if (Parser.m_Text[Parser.m_Index] == '.') Parser.m_Index++;
	while (isdigit(Parser.m_Text[Parser.m_Index])) Parser.m_Index++;

	if (Parser.m_Index - index == 0)
	{
		snprintf(buffer, 40, "Number expected but not found!");
		longjmp(ebuf, 1);
	}
	char buffer[32] = { 0 };
	memcpy(buffer, &Parser.m_Text[index], Parser.m_Index - index);

	return atof(buffer);
	return 0;
}

double EvaluateSubtree(ASTNode* ast)
{
	if (ast == NULL)
	{
		snprintf(buffer, 40, "Incorrect syntax tree!");
		longjmp(ebuf, 1);
	}
	if (ast->Type == NumberValue)
		return ast->Value;
	else if (ast->Type == UnaryMinus)
		return -EvaluateSubtree(ast->Left);
	else
	{
		double v1 = EvaluateSubtree(ast->Left);
		double v2 = EvaluateSubtree(ast->Right);
		switch (ast->Type)
		{
		case OperatorPlus:  return v1 + v2;
		case OperatorMinus: return v1 - v2;
		case OperatorMul:   return v1 * v2;
		case OperatorDiv:   return v1 / v2;
		}
	}
	snprintf(buffer, 40, "Incorrect syntax tree!");
	longjmp(ebuf, 1);
}
void Evaluate(const char* text)
{
	ASTNode* ast = NULL;
	if (!(setjmp(ebuf)))
	{
		ast = Parse(text);
		double val = EvaluateSubtree(ast);
		printf("\n\t\t\t\t%s = %f\n", text, val);
	}
	else
		printf("%s\n", buffer);
	FreeNode(ast);
}

int main(void)
{

	char expression[100];
	while (1)
	{
		printf("\n\n\n\t\t\tEnter your expression: ");
		gets_s(expression, 100);
		Evaluate(expression);
		puts("ENTER to continue");
		if (getchar() != '\n')
			break;
	}
	system("Pause");
	return 0;
}
