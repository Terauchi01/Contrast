# contrast

## 概要
Contrast は 5x5 のボード上で黒 (X) と白 (O) がゴール列を目指しつつ、タイルを設置して移動方向を変化させる研究用プロジェクトです。タイル種別 (Black/Gray/None) によって 4 方向・斜め・全方向の移動が切り替わり、自駒の飛び越しは許される一方で敵駒は飛び越せません。

## ディレクトリ構成
```
contrast/
├── apps/                # GUI/CLI/実験ツール一式
│   ├── cli_play/        # CLI で人間 vs AI 対局
│   ├── cli_selfplay/    # CLI 自動対局 (雛形)
│   ├── gui_debug/       # ImGui + GLFW のデバッグ GUI (AI vs AI)
│   ├── gui_play/        # 人間 vs AI GUI (フェーズ式操作)
│   ├── train_ntuple/    # N-tuple 学習実験
│   ├── eval_ntuple/     # 学習済み評価関数の検証
│   ├── ntuple_designer/ # N-tuple 配置の可視化
│   ├── test_rule_based/ # ルールベース AI の大量対戦ベンチ
│   ├── test_rulebased/  # ルールベース AI の詳細ログ検証
│   ├── test_rulebased_analysis/
│   ├── debug_rulebased/
│   ├── compare_memory.cpp / estimate_memory.cpp
│   ├── enumerate_3x3.cpp / enumerate_real_3x3.cpp
│   ├── visualize_patterns.cpp / visualize_rotations.cpp
│   └── ...              # 研究補助の単機能ツール
├── core/                # Board, GameState, Rules などの基礎ロジック
├── engine/              # Random / Greedy / RuleBased / NTuple / MCTS(雛形)
├── common/              # サーバー/クライアント共通のテキストプロトコル
├── server/              # TCP サーバー (既定ポート 8765)
├── client/              # CLI クライアント (手動入力 + ランダム/ルールベース AI)
├── tests/               # GoogleTest ベースの単体テスト
├── scripts/             # デモ・自動実験スクリプト
├── docs/                # GUI 操作手順や N-tuple メモ
├── web/                 # Web UI 実験コード
├── build/               # CMake ビルド成果物 (生成物)
├── CMakeLists.txt       # ルート設定
└── README.md            # 本ファイル
```

### apps ディレクトリの詳細
- `gui_play/`: 人間 vs AI。マウス操作で駒→目的地→タイル設置を段階的に行う。
- `gui_debug/`: AI vs AI 確認。合法手リストや在庫表示、NTuple 重み読み込み GUI を備える。
- `train_ntuple/`: `train_alternating.cpp` などで N-tuple パラメータ更新を実行。
- `eval_ntuple/`: 学習済み重みの精度検証や可視化。
- `ntuple_designer/`: 特徴パターン構築のためのインタラクティブツール。
- `test_rule_based/` と `test_rulebased/`: どちらもルールベース AI 評価だが、前者は Random/Greedy との対戦統計、後者は詳細ログ付きの挙動観察用途。

### server / client / common
- `common/protocol.hpp` が MOVE/STATE メッセージを定義。
- `server/contrast_server`: 盤面を保持し、合法手チェック後に全クライアントへ StateSnapshot を配信。
- `client/contrast_client`: サーバーへ接続し CLI で手入力。第 3 引数に `random` / `rule` を渡すと内蔵 AI が自動送信。

### その他
- `core/` と `engine/` はそれぞれ `include/contrast`, `include/contrast_ai` を公開し、`libcontrast_core.a`, `libcontrast_engine.a` にビルドされます。
- `docs/GUI_USAGE.md` で GUI の操作手順、`docs/ntuple_patterns.md` で特徴テンプレートを整理。
- `web/` は将来的なブラウザ UI の検証用コードです (現在は未接続)。

## ビルド
```bash
mkdir -p build
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --target all
```

## 主な実行方法
- GUI (人間 vs AI): `./apps/gui_play/gui_play`
- GUI (AI vs AI デバッグ): `./apps/gui_debug/gui_debug`
- サーバー: `./server/contrast_server`
- クライアント: `./client/contrast_client <role> <name> <model>`
  - `role`: `X`, `O`, `spectator`, `-`
  - `model`: `random`, `rule`, `-` (手動)

## テスト
```bash
cd build
ctest                # もしくは ./tests/test_rules 等を個別に実行
```

## AI 実装一覧
- **RandomPolicy** (`engine/src/random_policy.cpp`): 合法手から一様ランダムで選択。
- **GreedyPolicy** (`engine/src/greedy_policy.cpp`): ゴール列に近づく手を優先、なければランダム。
- **RuleBasedPolicy** (`engine/src/rule_based_policy.cpp`): 優先度付きルールで侵攻/防御を切り替える。
- **NTuplePolicy** (`engine/src/ntuple.cpp` + `train_ntuple/`): N-tuple 評価関数を読み込み、GUI やサーバークライアントから利用予定。
- **MCTS (stub)** (`engine/src/mcts.cpp`): 今後拡張予定のモンテカルロ木探索骨格。

## 重複・整理候補
- `tests/test_ai.cpp.bak`, `tests/test_ai.cpp.bak2`
- `tests/test_ntuple.cpp.bak`, `tests/test_ntuple.cpp.bak2`
- `tests/test_rules.cpp.bak`
  - いずれも現行テストファイルのバックアップコピーであり、内容が重複しています。履歴保持が不要であれば削除して構いません。
- `apps/test_rule_based/` と `apps/test_rulebased/` は役割が近いため、片方に機能統合するか README に目的差を明記しておくと保守が楽になります。

上記以外に明確な役割重複や不要ファイルは確認できませんでした。バックアップ整理とテストアプリ統合を検討してください。
