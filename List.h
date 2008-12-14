/*
 * Copyright (C) 2008, OctaneSnail <os@v12pwr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIST_H
#define LIST_H

template <class T> class List;

template <class T> class Node
{
	friend class List<T>;

public:
	Node (void) : next (NULL), prev (NULL) { }

	void Unlink (void)
	{
		next->prev = prev;
		prev->next = next;
	}

private:
	Node (Node *next, Node *prev) : next (next), prev (prev) { }

	Node *next;
	Node *prev;
};

template <class T> class List
{
public:
	List (bool auto_clear = false) : anchor (&anchor, &anchor), clear (auto_clear) { }
	~List() { if (clear) Clear (); }

	bool IsEmpty (void) { return anchor.next == &anchor; }

	T *First (void) { return Next (&anchor); }
	T *Last (void) { return Prev (&anchor); }

	void InsertFirst (Node<T> *n);
	void InsertLast (Node<T> *n);

	T *UnlinkFirst (void);
	T *UnlinkLast (void);

	T *Next (Node<T> *n);
	T *Prev (Node<T> *n);

	void Clear (void);

private:
	Node<T> anchor;
	bool clear;
};

#include "List.cpp"

#endif // LIST_H
