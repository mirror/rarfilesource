/*
 * Copyright (C) 2008-2012, OctaneSnail <os@v12pwr.com>
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

template <class T> void List<T>::InsertFirst (Node<T> *n)
{
	n->next = anchor.next;
	n->prev = &anchor;

	anchor.next->prev = n;
	anchor.next = n;
}

template <class T> void List<T>::InsertLast (Node<T> *n)
{
	n->next = &anchor;
	n->prev = anchor.prev;

	anchor.prev->next = n;
	anchor.prev = n;
}

template <class T> T *List<T>::UnlinkFirst (void)
{
	Node<T> *n = First ();
	if (!n)
		return NULL;
	n->Unlink ();
	return (T *) n;
}

template <class T> T *List<T>::UnlinkLast (void)
{
	Node<T> *n = Last ();
	if (!n)
		return NULL;
	n->Unlink ();
	return (T *) n;
}

template <class T> T *List<T>::Next (Node<T> *n)
{
	if (n->next == &anchor)
		return NULL;
	return (T *) n->next;
}

template <class T> T *List<T>::Prev (Node<T> *n)
{
	if (n->prev == &anchor)
		return NULL;
	return (T *) n->prev;
}

template <class T> void List<T>::Clear ()
{
	T *node;

	while (node = UnlinkFirst ())
		delete node;
}
