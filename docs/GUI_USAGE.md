# Contrast GUI 使用方法

## 起動

```bash
./build/apps/gui_debug/gui_debug
```

## 画面説明

ウィンドウは左右2列に分かれています:

### 左側: ゲームボード (5x5)
- **セル表示**: `駒タイル (座標)`
  - 駒: `X` = Black, `O` = White, スペース = 空
  - タイル: `B` = Black tile, `G` = Gray tile, なし = White tile
  - 例: `X (0,0)` = Black駒がマス(0,0)にいる
  
- **背景色**:
  - 白背景: White tile (4方向直交移動)
  - 黒背景: Black tile (4方向斜め移動)
  - 灰背景: Gray tile (8方向移動)
  
- **操作**:
  - 自分の駒をクリック → 選択 (青く光る)
  - 移動可能なマスをクリック → 移動実行 (緑で表示)
  - タイル配置が必要な場合 → ポップアップで選択

### 右側: コントロールパネル

#### 現在の状況
- **Turn**: 現在のプレイヤー (Black / White)
- **Next move (auto)**: 次の合法手の例
- **Apply Next Move**: 最初の合法手を自動適用
- **Legal moves (sample)**: 合法手のリスト (最大50手表示)
- **Inventory**: 現在のプレイヤーのタイル在庫 (Black/Gray)

#### AI vs AI モード
1. **Black**: AI選択 (Random / Greedy)
2. **White**: AI選択 (Random / Greedy)
3. **Delay (s)**: AI手番の間隔 (0.1秒 ~ 2.0秒)
4. **Start AI vs AI**: AI対戦開始
5. **Stop AI vs AI**: AI対戦停止
6. **Reset Game**: ゲームをリセット

## AI タイプ説明

### Random
- ランダムに合法手を選択
- ベースライン評価用

### Greedy
- 常に前進する手を優先
- Black: 下方向 (row 4 を目指す)
- White: 上方向 (row 0 を目指す)
- Random相手に96%勝率

## 操作例

### 手動プレイ
1. GUIを起動
2. 左側のボードで自分の駒をクリック
3. 緑色に光った移動先をクリック
4. (必要なら) タイル配置を選択

### AI対戦を観る
1. GUIを起動
2. 右側パネルで Black: Greedy, White: Random を選択
3. Delay を 0.5s に設定
4. "Start AI vs AI" をクリック
5. リアルタイムで対戦が進む
6. 勝敗が決まったら自動停止

## トラブルシューティング

### 盤面が見えない
- ウィンドウサイズを広げてください (1200x800推奨)
- ウィンドウ全体が使われているか確認

### GUIが起動しない
- macOSではOpenGL 2.1にフォールバックします
- SSH経由では起動できません (ローカル実行が必要)

### AI対戦が止まらない
- "Stop AI vs AI" ボタンで停止できます
- 200手で自動的に引き分け判定されます
