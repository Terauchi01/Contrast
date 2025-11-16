# N-tuple Network Training

このプログラムは、Contrastゲーム用のN-tupleネットワークを時間差学習(TD学習)で訓練します。

## 概要

- **アルゴリズム**: TD(0)学習 + ε-greedy探索
- **ネットワーク構造**: 16個の2x2タプル(重複配置で5x5ボード全体をカバー)
- **パラメータ数**: 104,976個の重み
- **学習方法**: セルフプレイによる強化学習

## ビルド

```bash
cd build
cmake ..
cmake --build . --target train_ntuple
```

## 使い方

### 基本的な使い方

```bash
./apps/train_ntuple/train_ntuple
```

デフォルトで10,000ゲームを訓練し、`ntuple_weights.bin`に保存します。

### オプション

```bash
./apps/train_ntuple/train_ntuple [options]
```

#### 利用可能なオプション

- `--games N`: 訓練ゲーム数(デフォルト: 10000)
- `--lr RATE`: 学習率(デフォルト: 0.01)
- `--discount GAMMA`: 割引率(デフォルト: 0.9)
- `--epsilon EPS`: 探索率(デフォルト: 0.1)
- `--save-interval N`: チェックポイント保存間隔(デフォルト: 1000)
- `--output PATH`: 出力ファイルパス(デフォルト: ntuple_weights.bin)
- `--help`: ヘルプメッセージを表示

### 使用例

#### 短時間のテスト

```bash
./apps/train_ntuple/train_ntuple --games 100 --epsilon 0.2 --output test.bin
```

#### 長時間の訓練

```bash
./apps/train_ntuple/train_ntuple --games 100000 --lr 0.005 --epsilon 0.05 --save-interval 5000 --output trained_weights.bin
```

#### 高探索率での訓練

```bash
./apps/train_ntuple/train_ntuple --games 50000 --epsilon 0.3 --lr 0.02
```

## アルゴリズムの詳細

### TD(0)学習

時間差学習は、ゲームの各状態の価値を、実際のゲーム結果に基づいて更新します。

1. **報酬設定**:
   - 黒勝利: +1.0
   - 白勝利: -1.0
   - 引き分け: 0.0

2. **価値更新**:
   - ゲーム終了から逆順に各状態を更新
   - TD誤差 = 目標値 - 現在の評価値
   - 重み更新: w += 学習率 × TD誤差

3. **ε-greedy探索**:
   - 確率εでランダムな手を選択(探索)
   - 確率(1-ε)で最良の手を選択(活用)

### ハイパーパラメータの推奨値

- **学習率(lr)**: 0.001 ~ 0.02
  - 小さい値: 安定だが学習が遅い
  - 大きい値: 学習が速いが不安定

- **割引率(discount)**: 0.9 ~ 0.99
  - 将来の報酬をどれだけ重視するか

- **探索率(epsilon)**: 0.05 ~ 0.3
  - 小さい値: より活用重視
  - 大きい値: より探索重視

## 出力形式

訓練中は100ゲームごとに進捗が表示されます:

```
Game    100/10000 | B:  49 W:  51 D:   0 | Avg moves:  42.5 | 100.0 games/s
```

- B: 黒の勝利数
- W: 白の勝利数
- D: 引き分け数
- Avg moves: 平均手数
- games/s: 秒あたりのゲーム数

チェックポイントも定期的に保存されます:

```
Saved checkpoint: ntuple_weights.bin.1000
Saved checkpoint: ntuple_weights.bin.2000
...
```

## 学習済み重みの使用

学習済みの重みは`NTupleNetwork::load()`で読み込めます:

```cpp
NTupleNetwork network;
network.load("ntuple_weights.bin");

// NTuplePolicyで使用
NTuplePolicy policy(network);
Move move = policy.pick(state);
```

## パフォーマンス

- Intel Core i7程度のCPUで約100ゲーム/秒
- 10,000ゲームの訓練は約1.5分
- 100,000ゲームの訓練は約15分

## トラブルシューティング

### 学習が収束しない

- 学習率を下げる(例: 0.005)
- 探索率を上げる(例: 0.2)
- 訓練ゲーム数を増やす

### 黒または白に極端に偏る

- 探索率を上げる
- 割引率を調整する
- 初期状態からの学習を確認

## 次のステップ

1. 学習済みネットワークをGreedyやRandomと対戦させて評価
2. GUIに統合してインタラクティブにテスト
3. より複雑なネットワーク構造を試す
4. 異なるハイパーパラメータで実験
