#include <iostream>
using namespace std;

struct Node
{
    int *keys;
    int t;                     // Grado mínimo del B+ Tree
    Node **childPointers;      // Punteros a los hijos
    Node *next;                // Enlace entre nodos hoja
    int n;                     // Número actual de claves
    bool leaf;                 // Verdadero si el nodo es hoja

    Node(int t, bool leaf)
    {
        this->t = t;
        this->leaf = leaf;
        keys = new int[(2 * t) - 1];
        childPointers = new Node *[2 * t];
        n = 0;
        next = nullptr;
    }
};

class BPlusTree
{
public:
    Node *root;
    int t;

    BPlusTree(int degree)
    {
        root = nullptr;
        t = degree;
    }

    Node *search(Node *node, int key)
    {
        if (!node) return nullptr;
        int i = 0;
        while (i < node->n && key > node->keys[i])
            i++;

        if (node->leaf)
            return (i < node->n && node->keys[i] == key) ? node : nullptr;

        return search(node->childPointers[i], key);
    }

    void insert(int key)
    {
        if (!root)
        {
            root = new Node(t, true);
            root->keys[0] = key;
            root->n = 1;
        }
        else
        {
            if (root->n == (2 * t) - 1)
            {
                Node *newRoot = new Node(t, false);
                newRoot->childPointers[0] = root;
                splitChild(newRoot, 0, root);
                root = newRoot;
            }
            insertNonFull(root, key);
        }
    }

    void insertNonFull(Node *node, int key)
    {
        int i = node->n - 1;
        if (node->leaf)
        {
            while (i >= 0 && key < node->keys[i])
            {
                node->keys[i + 1] = node->keys[i];
                i--;
            }
            node->keys[i + 1] = key;
            node->n++;
        }
        else
        {
            while (i >= 0 && key < node->keys[i])
                i--;
            i++;
            if (node->childPointers[i]->n == (2 * t) - 1)
            {
                splitChild(node, i, node->childPointers[i]);
                if (key > node->keys[i])
                    i++;
            }
            insertNonFull(node->childPointers[i], key);
        }
    }

    void splitChild(Node *parentNode, int childIndex, Node *childNode)
    {
        Node *newNode = new Node(t, childNode->leaf);
        newNode->n = t - 1;

        for (int j = 0; j < t - 1; j++)
            newNode->keys[j] = childNode->keys[j + t];

        if (!childNode->leaf)
        {
            for (int j = 0; j < t; j++)
                newNode->childPointers[j] = childNode->childPointers[j + t];
        }
        else
        {
            newNode->next = childNode->next;
            childNode->next = newNode;
        }

        childNode->n = t - 1;

        for (int j = parentNode->n; j >= childIndex + 1; j--)
            parentNode->childPointers[j + 1] = parentNode->childPointers[j];

        parentNode->childPointers[childIndex + 1] = newNode;

        for (int j = parentNode->n - 1; j >= childIndex; j--)
            parentNode->keys[j + 1] = parentNode->keys[j];

        parentNode->keys[childIndex] = childNode->keys[t - 1];
        parentNode->n++;
    }

    void remove(int key)
    {
        if (!root)
        {
            cout << "El árbol está vacío." << endl;
            return;
        }
        removeKey(root, key);

        if (root->n == 0)
        {
            Node *oldRoot = root;
            root = root->leaf ? nullptr : root->childPointers[0];
            delete oldRoot;
        }
    }

    void removeKey(Node *node, int key)
    {
        int idx = findKeyIndex(node, key);

        if (node->leaf)
        {
            if (idx < node->n && node->keys[idx] == key)
                removeFromLeaf(node, idx);
            return;
        }

        if (idx < node->n && node->keys[idx] == key)
            removeFromInternal(node, idx);
        else
        {
            bool isLastChild = (idx == node->n);
            Node *childNode = node->childPointers[idx];

            if (childNode->n < t)
                fill(node, idx);

            if (isLastChild && idx > node->n)
                removeKey(node->childPointers[idx - 1], key);
            else
                removeKey(node->childPointers[idx], key);
        }
    }

    void removeFromLeaf(Node *node, int idx)
    {
        for (int i = idx + 1; i < node->n; ++i)
            node->keys[i - 1] = node->keys[i];
        node->n--;
    }

    void removeFromInternal(Node *node, int idx)
    {
        int key = node->keys[idx];

        if (node->childPointers[idx]->n >= t)
        {
            int pred = getPredecessor(node, idx);
            node->keys[idx] = pred;
            removeKey(node->childPointers[idx], pred);
        }
        else if (node->childPointers[idx + 1]->n >= t)
        {
            int succ = getSuccessor(node, idx);
            node->keys[idx] = succ;
            removeKey(node->childPointers[idx + 1], succ);
        }
        else
        {
            merge(node, idx);
            removeKey(node->childPointers[idx], key);
        }
    }

    int getPredecessor(Node *node, int idx)
    {
        Node *cur = node->childPointers[idx];
        while (!cur->leaf)
            cur = cur->childPointers[cur->n];
        return cur->keys[cur->n - 1];
    }

    int getSuccessor(Node *node, int idx)
    {
        Node *cur = node->childPointers[idx + 1];
        while (!cur->leaf)
            cur = cur->childPointers[0];
        return cur->keys[0];
    }

    void fill(Node *node, int idx)
    {
        if (idx != 0 && node->childPointers[idx - 1]->n >= t)
            borrowFromPrev(node, idx);
        else if (idx != node->n && node->childPointers[idx + 1]->n >= t)
            borrowFromNext(node, idx);
        else
        {
            if (idx != node->n)
                merge(node, idx);
            else
                merge(node, idx - 1);
        }
    }

    void borrowFromPrev(Node *node, int idx)
    {
        Node *child = node->childPointers[idx];
        Node *sibling = node->childPointers[idx - 1];

        for (int i = child->n - 1; i >= 0; --i)
            child->keys[i + 1] = child->keys[i];

        if (!child->leaf)
        {
            for (int i = child->n; i >= 0; --i)
                child->childPointers[i + 1] = child->childPointers[i];
        }

        child->keys[0] = node->keys[idx - 1];

        if (!node->leaf)
            child->childPointers[0] = sibling->childPointers[sibling->n];

        node->keys[idx - 1] = sibling->keys[sibling->n - 1];

        child->n += 1;
        sibling->n -= 1;
    }

    void borrowFromNext(Node *node, int idx)
    {
        Node *child = node->childPointers[idx];
        Node *sibling = node->childPointers[idx + 1];

        child->keys[child->n] = node->keys[idx];

        if (!(child->leaf))
            child->childPointers[child->n + 1] = sibling->childPointers[0];

        node->keys[idx] = sibling->keys[0];

        for (int i = 1; i < sibling->n; ++i)
            sibling->keys[i - 1] = sibling->keys[i];

        if (!sibling->leaf)
        {
            for (int i = 1; i <= sibling->n; ++i)
                sibling->childPointers[i - 1] = sibling->childPointers[i];
        }

        child->n += 1;
        sibling->n -= 1;
    }

    void merge(Node *node, int idx)
    {
        Node *child = node->childPointers[idx];
        Node *sibling = node->childPointers[idx + 1];

        child->keys[t - 1] = node->keys[idx];

        for (int i = 0; i < sibling->n; ++i)
            child->keys[i + t] = sibling->keys[i];

        if (!child->leaf)
        {
            for (int i = 0; i <= sibling->n; ++i)
                child->childPointers[i + t] = sibling->childPointers[i];
        }

        for (int i = idx + 1; i < node->n; ++i)
            node->keys[i - 1] = node->keys[i];

        for (int i = idx + 2; i <= node->n; ++i)
            node->childPointers[i - 1] = node->childPointers[i];

        child->n += sibling->n + 1;
        node->n--;

        delete sibling;
    }

    int findKeyIndex(Node *node, int key)
    {
        int idx = 0;
        while (idx < node->n && node->keys[idx] < key)
            ++idx;
        return idx;
    }
};

int main()
{
    BPlusTree bPlusTree(3);

    bPlusTree.insert(10);
    bPlusTree.insert(20);
    bPlusTree.insert(5);
    bPlusTree.insert(6);
    bPlusTree.insert(12);
    bPlusTree.insert(30);
    bPlusTree.insert(7);
    bPlusTree.insert(17);

    Node *result = bPlusTree.search(bPlusTree.root, 6);
    cout << (result ? "Clave encontrada" : "Clave no encontrada") << endl;

    bPlusTree.remove(6);

    result = bPlusTree.search(bPlusTree.root, 6);
    cout << (result ? "Clave encontrada" : "Clave no encontrada") << endl;

    return 0;
}
