#ifndef __EXPRESSIONS_H
#define __EXPRESSIONS_H

#include <glib.h>

#include "undo.h"

template <typename Type>
class ValueStack {
	class UndoTokenPush : public UndoToken {
		ValueStack<Type> *stack;

		Type	value;
		int	index;

	public:
		UndoTokenPush(ValueStack<Type> *_stack,
			      Type _value, int _index = 1)
			     : stack(_stack), value(_value), index(_index) {}

		void
		run(void)
		{
			stack->push(value, index);
		}
	};

	class UndoTokenPop : public UndoToken {
		ValueStack<Type> *stack;

		int index;

	public:
		UndoTokenPop(ValueStack<Type> *_stack, int _index = 1)
			    : stack(_stack), index(_index) {}

		void
		run(void)
		{
			stack->pop(index);
		}
	};

	int size;

	Type *stack;
	Type *top;

public:
	ValueStack(int _size = 1024) : size(_size)
	{
		top = stack = new Type[size];
	}

	~ValueStack()
	{
		delete stack;
	}

	inline int
	items(void)
	{
		return top - stack;
	}

	inline Type &
	push(Type value, int index = 1)
	{
		for (int i = -index + 1; i; i++)
			top[i+1] = top[i];

		top++;
		return peek(index) = value;
	}
	inline void
	undo_push(Type value, int index = 1)
	{
		undo.push(new UndoTokenPush(this, value, index));
	}

	inline Type
	pop(int index = 1)
	{
		Type v = peek(index);

		top--;
		while (--index)
			top[-index] = top[-index + 1];

		return v;
	}
	inline void
	undo_pop(int index = 1)
	{
		undo.push(new UndoTokenPop(this, index));
	}

	inline Type &
	peek(int index = 1)
	{
		return top[-index];
	}
};

/*
 * Arithmetic expression stacks
 */
extern class Expressions {
public:
	/* reflects also operator precedence */
	enum Operator {
		OP_NIL = 0,
		OP_POW,		// ^*
		OP_MUL,		// *
		OP_DIV,		// /
		OP_MOD,		// ^/
		OP_SUB,		// -
		OP_ADD,		// +
		OP_AND,		// &
		OP_OR,		// #
				// pseudo operators:
		OP_NEW,
		OP_BRACE,
		OP_LOOP,
		OP_NUMBER
	};

private:
	ValueStack<gint64>	numbers;
	ValueStack<Operator>	operators;

public:
	Expressions() : num_sign(1), radix(10) {}

	gint num_sign;
	void set_num_sign(gint sign);

	gint radix;
	void set_radix(gint r);

	gint64 push(gint64 number);

	inline gint64
	peek_num(int index = 1)
	{
		return numbers.peek(index);
	}
	gint64 pop_num(int index = 1);
	gint64 pop_num_calc(int index, gint64 imply);
	inline gint64
	pop_num_calc(int index = 1)
	{
		return pop_num_calc(index, num_sign);
	}

	gint64 add_digit(gchar digit);

	Operator push(Operator op);
	Operator push_calc(Operator op);
	inline Operator
	peek_op(int index = 1)
	{
		return operators.peek(index);
	}
	Operator pop_op(int index = 1);

	void eval(bool pop_brace = false);

	int args(void);

	void discard_args(void);

private:
	void calc(void);

	int first_op(void);
} expressions;

#endif
