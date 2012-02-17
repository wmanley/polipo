/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
 * Polipo stores almost all of its data in "atoms".  This might be part of an
 * HTTP request/response or anything else including binary data.
 *
 * Atoms are stored in a hash table keyed on their contents, such that they
 * can be looked up by content.  This allows Polipo to save memory by not
 * storing duplicate content.  It is probably used for something else as
 * well....
 *
 * An object like a file that has been downloaded is stored in chunks, each
 * chunk being an atom and the entire file represented as an AtomList.
 */

typedef struct _Atom {
    unsigned int refcount;
    // The next atom in the hash-table chain
    struct _Atom *next;

    unsigned short length;
    char string[1];
} AtomRec, *AtomPtr;
typedef const AtomRec *ConstAtomPtr;

typedef struct _AtomList {
    int length;
    int size;
    AtomPtr *list;
} AtomListRec, *AtomListPtr;

#define LOG2_ATOM_HASH_TABLE_SIZE 10
#define LARGE_ATOM_REFCOUNT 0xFFFFFF00U

extern int used_atoms;

/**
 * Initialise the atom hash-table
 */
void initAtoms(void);

/**
 * Create an atom from the provided data and intern it in the hash table of all
 * atoms.
 *
 * The argument to internAtom must be a NUL terminated string.  This is not the
 * case with internAtomN where the length of the data (not including any
 * terminating NULs in the case of a string) is given by the second argument.
 * This is typically useful for storing binary data.
 *
 * internAtomLowerN is like internAtomLower but will store a lower case version
 * of the given data.  This can be useful if in the future we wish to do a
 * case-insensitive comparison of this data with some other.
 *
 * internAtomF and internAtomError will intern an atom with the given string
 * expanded according to the usual printf rules.  internAtomError will
 * additionally suffix the expanded string with ": <description of error>".
 *
 * The returned atom will either have refcount=1 or possibly higher if the
 * provided data is identical to some other that has previously been stored in
 * an atom (See hash table discussion above).
 */
AtomPtr internAtom(const char *string);
AtomPtr internAtomN(const char *string, int n);
AtomPtr internAtomLowerN(const char *string, int n);
AtomPtr internAtomF(const char *format, ...)
     ATTRIBUTE ((format (printf, 1, 2)));
AtomPtr internAtomError(int e, const char *f, ...)
     ATTRIBUTE ((format (printf, 2, 3)));

/**
 * Create a new atom consisting of the contents of atom with string appended.
 *
 * Note: this function returns a new atom and has no effect on the old atom,
 * essentially copying it.  This means that atomCat(&x, "") will return a
 * copy of x.
 */
AtomPtr atomCat(ConstAtomPtr atom, const char *string);

/**
 * Split atom at the first instance of charactor c "returning" new atoms into
 * the locations given by return1 and return2.
 *
 * *return1 and *return2 must be uninitialized AtomPtrs.
 */
int atomSplit(ConstAtomPtr atom, char c, AtomPtr *return1, AtomPtr *return2);

/**
 * Manipulate the Atom reference count.
 *     retainAtom increments the reference count
 *     releaseAtom decrements the reference count possibly freeing resources
 *         if possible.
 */
AtomPtr retainAtom(AtomPtr atom);
void releaseAtom(AtomPtr atom);

/**
 * Return the string contained within the atom or "(null)".
 *
 * The contents of the returned data should not be modified.
 */
const char *atomString(ConstAtomPtr) ATTRIBUTE ((pure));

/**
 * Creates/destroys an atom list.  Atom lists are created from the provided
 * array of atoms of length n.
 *
 * NOTE: makeAtomList takes ownership of the provided atoms and will *not*
 *       increment the atom's reference count.  This is in contrast to
 *       destroyAtomList which will decrement the ref count on each atom when
 *       it is called.
 */
AtomListPtr makeAtomList(AtomPtr *atoms, int n);
void destroyAtomList(AtomListPtr list);

/**
 * Returns 1 if the atom is a member of the given list, 0 otherwise.
 */
int atomListMember(ConstAtomPtr atom, AtomListPtr list)
    ATTRIBUTE ((pure));

/**
 * Append Atom atom to AtomList list.
 *
 * NOTE: Much like makeAtomList this function will not affect the refcount of
 *       the given atom.
 */
void atomListCons(AtomPtr atom, AtomListPtr list);
