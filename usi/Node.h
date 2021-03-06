#pragma once

#include <atomic>
#include <vector>
#include <memory>

#include "cppshogi.h"

struct uct_node_t;
struct child_node_t {
	child_node_t() : move_count(0), win(0.0f), nnrate(0.0f) {}
	child_node_t(const Move move)
		: move(move), move_count(0), win(0.0f), nnrate(0.0f) {}
	// ムーブコンストラクタ
	child_node_t(child_node_t&& o) noexcept
		: move(o.move), move_count(0), win(0.0f), nnrate(0.0f), node(std::move(o.node)) {}
	// ムーブ代入演算子
	child_node_t& operator=(child_node_t&& o) noexcept {
		move = o.move;
		move_count = (int)o.move_count;
		win = (float)o.win;
		nnrate = (float)o.nnrate;
		node = std::move(o.node);
		return *this;
	}

	// ノードの展開
	uct_node_t* ExpandNode(const Position* pos);

	Move move;                   // 着手する座標
	std::atomic<int> move_count; // 探索回数
	std::atomic<float> win;      // 勝った回数
	float nnrate;                // ニューラルネットワークでのレート
	std::unique_ptr<uct_node_t> node; // 子ノードへのポインタ
};

struct uct_node_t {
	uct_node_t()
		: move_count(0), win(0.0f), evaled(false), value_win(0.0f), visited_nnrate(0.0f), child_num(0) {}
	// 合法手の一覧で初期化する
	uct_node_t(MoveList<Legal>& ml)
		: move_count(0), win(0.0f), evaled(false), value_win(0.0f), visited_nnrate(0.0f),
		child_num(ml.size()), child(std::make_unique<child_node_t[]>(ml.size())) {
		auto* child_node = child.get();
		for (; !ml.end(); ++ml) child_node++->move = ml.move();
	}

	// 子ノード一つのみで初期化する
	void CreateSingleChildNode(const Move move) {
		child_num = 1;
		child = std::make_unique<child_node_t[]>(1);
		child[0].move = move;
	}
	// 合法手の一覧で初期化する
	void CreateChildNode(MoveList<Legal>& ml) {
		child_num = ml.size();
		child = std::make_unique<child_node_t[]>(ml.size());
		auto* child_node = child.get();
		for (; !ml.end(); ++ml) child_node++->move = ml.move();
	}

	// 1つを除くすべての子を削除する
	// 1つも見つからない場合、新しいノードを作成する
	// 残したノードを返す
	uct_node_t* ReleaseChildrenExceptOne(const Move move);

	void Lock() {
		mtx.lock();
	}
	void UnLock() {
		mtx.unlock();
	}

	std::atomic<int> move_count;
	std::atomic<float> win;
	std::atomic<bool> evaled;      // 評価済か
	std::atomic<float> value_win;
	std::atomic<float> visited_nnrate;
	int child_num;                         // 子ノードの数
	std::unique_ptr<child_node_t[]> child; // 子ノードの情報

	std::mutex mtx;
};

class NodeTree {
public:
	~NodeTree() { DeallocateTree(); }
	// ツリー内の位置を設定し、ツリーの再利用を試みる
	// 新しい位置が古い位置と同じゲームであるかどうかを返す（いくつかの着手動が追加されている）
	// 位置が完全に異なる場合、または以前よりも短い場合は、falseを返す
	bool ResetToPosition(const Key starting_pos_key, const std::vector<Move>& moves);
	uct_node_t* GetCurrentHead() const { return current_head_; }

private:
	void DeallocateTree();
	// 探索を開始するノード
	uct_node_t* current_head_ = nullptr;
	// ゲーム木のルートノード
	std::unique_ptr<uct_node_t> gamebegin_node_;
	// 以前の局面
	Key history_starting_pos_key_;
};
