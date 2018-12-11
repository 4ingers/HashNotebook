#pragma once

#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <fstream>
#include <string>
#include "Contact.h"

#define DEFAULT_SIZE 128

enum { INS, DEL };

template <class TKey, typename TData>
struct Node {
	TKey key;
	TData data;

	Node() = default;
	Node(const TKey& nKey, const TData& nData) : key(nKey), data(nData) { };
	bool operator ==(const Node<TKey, TData> other)	{return key == other.key;}
};


template <typename TKey, typename TData>
class HashTab {
public:
	class HIterator {
	public:
		HIterator() = default;
		HIterator(const HashTab<TKey, TData>* table, int iVec, int iList);
		HIterator& operator ++();
		Node<TKey, TData> operator *() { return *curNode; }
		bool operator==(const HIterator& rhd) const {return curNode == rhd.curNode;}
		bool operator!=(const HIterator& rhd) const {return curNode != rhd.curNode;}
		void print();
	private:
		const Node<TKey, TData> *curNode;
		const HashTab<TKey, TData> *curTable;
		int curVectorI = 0,
				curListI   = 0;
	};

	HashTab() { tableStore.resize(DEFAULT_SIZE); }
	HashTab(int size) { tableStore.resize(size); }

	int hash(const std::string&) const;
	void insert(const TKey&, const TData&);

	HIterator begin() const;
	HIterator end() const {	return HIterator(this, tableStore.size(), 0); }
	HIterator find(const TKey&) const;
	HIterator find_value(const std::string&);
	void erase(const TKey&);
	void undo(int m);
	void print() {
		for (auto it : (*this) )	std::cout << it.data << std::endl;
	}

	template <typename UKey, class UData>
	friend std::ofstream& operator <<(std::ofstream&, const HashTab<UKey,UData>&);

private:
	std::vector<std::list<Node<TKey, TData>>> tableStore;
	std::stack<int> cancelArr;
	std::stack<Node<TKey, TData>> insertHistory;
	std::stack<Node<TKey, TData>> removeHistory;
};



template <typename TKey, typename TData>
int HashTab<TKey, TData>::hash(const std::string& key) const {
	int result = 0;
	for (int i = 0; i < key.length(); ++i)
		result += key[i] * key.length() % tableStore.size();
	return result % tableStore.size();
}


template <typename TKey, typename TData>
void HashTab<TKey, TData>::insert(const TKey& key, const TData& data) {
	int hKey = hash(key);
	Node<TKey, TData> newNode(key, data);
	auto it = std::find(tableStore[hKey].begin(), tableStore[hKey].end(), newNode);
	if (tableStore[hKey].end() == it)
		tableStore[hKey].push_back(newNode);
	else
		(*it).data = data;
	cancelArr.push(INS);
	insertHistory.push(newNode);
}


template <typename TKey, typename TData>
typename HashTab<TKey, TData>::HIterator HashTab<TKey, TData>::begin() const {
	int i;
	for (i = 0; i != tableStore.size() && tableStore[i].empty(); ++i);
	return HIterator(this, i, 0);
}


template <typename TKey, typename TData>
HashTab<TKey, TData>::HIterator::HIterator(
	const HashTab<TKey, TData>* table, int iHash, int iList
) {
	this->curTable = table;
	curNode = nullptr;
	if (iHash >= curTable->tableStore.size())
		return;
	this->curVectorI = iHash;
	if (iList >= curTable->tableStore[iHash].size())
		return;
	this->curListI = iList;
	if (curTable->tableStore[iHash].begin() != curTable->tableStore[iHash].end())
		curNode = &(*next(curTable->tableStore[iHash].begin(), iList));
}


template <typename TKey, typename TData>
typename HashTab<TKey, TData>::HIterator& HashTab<TKey, TData>::HIterator::operator++ () {
	if (curTable->tableStore[curVectorI].size() - 1 == curListI) {
		int tmpIndex = curVectorI;
		for (int i = curVectorI + 1; i < curTable->tableStore.size(); ++i) {
			if (!curTable->tableStore[i].empty()) {
				curVectorI = i;
				break;
			}
		}
		if (tmpIndex == curVectorI) {
			curNode = nullptr;
			return *this;
		}
		curListI = 0;
	}
	else
		++curListI;
	if (curNode == nullptr)
		return *this;
	curNode = &(*next(curTable->tableStore[curVectorI].begin(), curListI));
	return *this;
}


template <typename TKey, typename TData>
typename HashTab<TKey, TData>::HIterator HashTab<TKey, TData>::find(const TKey& key) const {
	int hKey = hash(key),
			iList = 0;
	for (auto jt : tableStore[hKey]) {
		if (key == jt.key)
			return HIterator(this, hKey, iList);
		++iList;
	}
	return HIterator(this, tableStore.size(), 0);
}

template <typename TKey, typename TData>
typename HashTab<TKey, TData>::HIterator
	HashTab<TKey, TData>::find_value(const std::string& subStr) {
	for (int iHash = 0; iHash < tableStore.size(); ++iHash) {
		int iList = 0;
		for (auto jt : tableStore[iHash]) {
			if (jt.data.find_by_param(subStr))
				return HIterator(this, iHash, iList);
			++iList;
		}
	}
	return HIterator(this, tableStore.size(), 0);
}

template <typename TKey, typename TData>
void HashTab<TKey, TData>::HIterator::print() {
	if (!(nullptr == curNode))
		cout << curNode->data;
}

template <typename TKey, typename TData>
void HashTab<TKey, TData>::erase(const TKey& key)
{
	int hKey = hash(key);
	for (auto jt : tableStore[hKey]) {
		if (key == jt.key) {
			removeHistory.push(jt);
			cancelArr.push(DEL);
			tableStore[hKey].remove(jt);
			return;
		}
	}
}

template <typename TKey, typename TData>
void HashTab<TKey, TData>::undo(int m) {
	int arrSize = cancelArr.size();
	for (int i = 0; i < m && i < arrSize ; ++i) {
		if (cancelArr.top() == DEL) {
			Node<TKey, TData> newNode(removeHistory.top().key, removeHistory.top().data);
			tableStore[hash(removeHistory.top().key)].push_back(newNode);
			removeHistory.pop();
			cancelArr.pop();
		}
		else if (cancelArr.top() == INS) {
			int hKey = hash(insertHistory.top().key);
			for (auto it : tableStore[hKey])
				if (insertHistory.top().key == it.key) {
					tableStore[hKey].remove(it);
					insertHistory.pop();
					cancelArr.pop();
					break;
				}
		}
	}
	while (!cancelArr.empty()) cancelArr.pop();
	while (!removeHistory.empty()) removeHistory.pop();
	while (!insertHistory.empty()) insertHistory.pop();
}


template <typename TKey, class TData>
std::ofstream& operator<< (std::ofstream& fout, const HashTab<TKey, TData>& right) {
	for (auto it : right.tableStore)
		for (auto jt : it)
			fout << jt.data;
	return fout;
}