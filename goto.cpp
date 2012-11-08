#include <bsd/sys/tree.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "sciteco.h"
#include "parser.h"
#include "undo.h"
#include "goto.h"

static gchar *skip_label = NULL;

class GotoTable {
	class UndoTokenSet : public UndoToken {
		GotoTable *table;

		gchar	*name;
		gint	pc;

	public:
		UndoTokenSet(GotoTable *_table, gchar *_name, gint _pc = -1)
			    : table(_table), name(g_strdup(_name)), pc(_pc) {}
		~UndoTokenSet()
		{
			g_free(name);
		}

		void
		run(void)
		{
			table->set(name, pc);
			g_free(name);
			name = NULL;
#if 0
			table->dump();
#endif
		}
	};

	class Label {
	public:
		RB_ENTRY(Label) nodes;

		gchar	*name;
		gint	pc;

		Label(gchar *_name, gint _pc = -1)
		     : name(g_strdup(_name)), pc(_pc) {}
		~Label()
		{
			g_free(name);
		}

		static inline int
		cmp(Label *l1, Label *l2)
		{
			return g_strcmp0(l1->name, l2->name);
		}
	};

	RB_HEAD(Table, Label) head;

	RB_GENERATE(Table, Label, nodes, Label::cmp);

public:
	GotoTable()
	{
		RB_INIT(&head);
	}

	~GotoTable()
	{
		clear();
	}

	gint
	remove(gchar *name)
	{
		gint existing_pc = -1;

		Label label(name);
		Label *existing = RB_FIND(Table, &head, &label);

		if (existing) {
			existing_pc = existing->pc;
			RB_REMOVE(Table, &head, existing);
			delete existing;
		}

		return existing_pc;
	}

	gint
	find(gchar *name)
	{
		Label label(name);
		Label *existing = RB_FIND(Table, &head, &label);

		return existing ? existing->pc : -1;
	}

	gint
	set(gchar *name, gint pc)
	{
		if (pc < 0)
			return remove(name);

		Label *label = new Label(name, pc);
		Label *existing;
		gint existing_pc = -1;

		existing = RB_FIND(Table, &head, label);
		if (existing) {
			existing_pc = existing->pc;
			g_free(existing->name);
			existing->name = label->name;
			existing->pc = label->pc;

			label->name = NULL;
			delete label;
		} else {
			RB_INSERT(Table, &head, label);
		}

#if 0
		dump();
#endif
		return existing_pc;
	}

	inline void
	undo_set(gchar *name, gint pc = -1)
	{
		undo.push(new UndoTokenSet(this, name, pc));
	}

	void
	clear(void)
	{
		Label *cur;

		while ((cur = RB_MIN(Table, &head))) {
			RB_REMOVE(Table, &head, cur);
			delete cur;
		}
	}

#if 0
	void
	dump(void)
	{
		Label *cur;

		RB_FOREACH(cur, Table, &head)
			g_printf("table[\"%s\"] = %d\n", cur->name, cur->pc);
		g_printf("---END---\n");
	}
#endif
};

static GotoTable table;

State *
StateLabel::custom(gchar chr)
{
	gchar *new_str;

	if (chr == '!') {
		table.undo_set(strings[0], table.set(strings[0], macro_pc));

		if (!g_strcmp0(strings[0], skip_label)) {
			undo.push_str(skip_label);
			g_free(skip_label);
			skip_label = NULL;

			undo.push_var<Mode>(mode);
			mode = MODE_NORMAL;
		}

		undo.push_str(strings[0]);
		g_free(strings[0]);
		strings[0] = NULL;

		return &states.start;
	}

	undo.push_str(strings[0]);
	new_str = g_strdup_printf("%s%c", strings[0] ? : "", chr);
	g_free(strings[0]);
	strings[0] = new_str;

	return this;
}
