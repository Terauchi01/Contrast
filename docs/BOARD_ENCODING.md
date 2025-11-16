# Board Encoding Specification

## Overview

このドキュメントでは、Contrastゲームの**完全な状態**を1次元配列としてエンコード/デコードする仕様を説明します。

**重要な変更（v2.0）**: 盤面だけでなく**タイル残数（インベントリ）**も含む29要素の拡張フォーマットに更新されました。これにより、戦略的に重要なタイル情報がAIやクライアントに提供されます。

## Encoding Format

### Array Structure (29 elements)

完全なゲーム状態は29要素の配列で表現されます：

```
[0-24]   盤面セル (25要素, 行優先順)
[25]     黒プレイヤーの残り黒タイル数 (0-3)
[26]     黒プレイヤーの残り灰色タイル数 (0-1)
[27]     白プレイヤーの残り黒タイル数 (0-3)
[28]     白プレイヤーの残り灰色タイル数 (0-1)
```

### Cell Encoding (0-8)

各セルは0-8の整数値にエンコードされます。

```
値 = occupant * 3 + tile
```

- `occupant`: 駒の状態
  - 0 = `Player::None` (空)
  - 1 = `Player::Black` (黒駒)
  - 2 = `Player::White` (白駒)

- `tile`: タイルの種類
  - 0 = `TileType::None` (タイルなし)
  - 1 = `TileType::Black` (黒タイル)
  - 2 = `TileType::Gray` (灰色タイル)

### Encoding Table

| 値 | occupant | tile | 説明 |
|----|----------|------|------|
| 0  | None     | None | 空のマス（タイルなし） |
| 1  | None     | Black | 空のマス + 黒タイル |
| 2  | None     | Gray | 空のマス + 灰色タイル |
| 3  | Black    | None | 黒駒（タイルなし） |
| 4  | Black    | Black | 黒駒 + 黒タイル |
| 5  | Black    | Gray | 黒駒 + 灰色タイル |
| 6  | White    | None | 白駒（タイルなし） |
| 7  | White    | Black | 白駒 + 黒タイル |
| 8  | White    | Gray | 白駒 + 灰色タイル |

### Tile Inventory Encoding

各プレイヤーは初期状態で以下のタイルを持ちます：
- **黒タイル**: 3枚（各プレイヤー）
- **灰色タイル**: 1枚（各プレイヤー）

配列の要素25-28でこれらの残数を表現：

| インデックス | 内容 | 範囲 | 説明 |
|--------------|------|------|------|
| 25 | Black player black tiles | 0-3 | 黒プレイヤーの残り黒タイル |
| 26 | Black player gray tiles | 0-1 | 黒プレイヤーの残り灰色タイル |
| 27 | White player black tiles | 0-3 | 白プレイヤーの残り黒タイル |
| 28 | White player gray tiles | 0-1 | 白プレイヤーの残り灰色タイル |

**戦略的重要性**: タイル残数は終盤戦略に極めて重要です。タイルを使い切った後の盤面評価や、最適な手の選択に影響します。

## Board Array Format

### 1D Array Layout

完全な状態は5x5 + 4 = **29要素**の1次元配列として表現されます。

- **サイズ**: 29要素（25盤面 + 4タイル情報）
- **順序**: 行優先順（row-major order）
- **インデックス**: 
  - Board: `[y * 5 + x]` where x,y ∈ [0,4]
  - Tiles: fixed positions [25-28]

### Coordinate System

```
  a   b   c   d   e    (files, x=0-4)
5 [ ] [ ] [ ] [ ] [ ]  (y=0, rank 5)
4 [ ] [ ] [ ] [ ] [ ]  (y=1, rank 4)
3 [ ] [ ] [ ] [ ] [ ]  (y=2, rank 3)
2 [ ] [ ] [ ] [ ] [ ]  (y=3, rank 2)
1 [ ] [ ] [ ] [ ] [ ]  (y=4, rank 1)
```

### Array Index Mapping

配列のインデックスと盤面座標・ゲーム状態の対応:

```
Index  Content           Position/Meaning
-----  ---------------   ------------------
  0    Board cell        a5 (top-left)
  1    Board cell        b5
  2    Board cell        c5
  3    Board cell        d5
  4    Board cell        e5
  5    Board cell        a4
  6    Board cell        b4
  ...
 20    Board cell        a1 (bottom-left)
 21    Board cell        b1
 22    Board cell        c1
 23    Board cell        d1
 24    Board cell        e1 (bottom-right)
 25    Tile inventory    Black player's black tiles (0-3)
 26    Tile inventory    Black player's gray tiles (0-1)
 27    Tile inventory    White player's black tiles (0-3)
 28    Tile inventory    White player's gray tiles (0-1)
```

## API Usage

### GET /api/game/:id/board_array

完全なゲーム状態を1次元配列として取得します。

**Response:**
```json
{
  "board_array": [3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,6,6,6,3,1,3,1],
  "encoding": "occupant*3+tile",
  "size": 29,
  "format": "row-major",
  "structure": {
    "board_cells": "[0-24]",
    "black_player_tiles": "[25-26]",
    "white_player_tiles": "[27-28]"
  },
  "tile_inventory": {
    "black_player": {"black": 3, "gray": 1},
    "white_player": {"black": 3, "gray": 1}
  }
}
```

### POST /api/game/:id/board_array

1次元配列から完全なゲーム状態を設定します。

**Request:**
```json
{
  "board_array": [3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,6,6,6,3,1,3,1]
}
```

**配列の構成:**
- `[0-24]`: 盤面セル（初期配置の例）
- `[25]`: 3 = 黒プレイヤーの黒タイル残数
- `[26]`: 1 = 黒プレイヤーの灰色タイル残数
- `[27]`: 3 = 白プレイヤーの黒タイル残数
- `[28]`: 1 = 白プレイヤーの灰色タイル残数

**Response:**
成功時はゲーム状態のJSONを返します。

## Code Examples

### C++ Encoder/Decoder

```cpp
// Encode a cell
int encode_cell(const Cell& cell) {
    int occ = static_cast<int>(cell.occupant);
    int tile = static_cast<int>(cell.tile);
    return occ * 3 + tile;
}

// Decode a value
bool decode_cell(int value, Cell& cell) {
    if (value < 0 || value > 8) return false;
    cell.occupant = static_cast<Player>(value / 3);
    cell.tile = static_cast<TileType>(value % 3);
    return true;
}

// Convert board to array
std::vector<int> board_to_array(const Board& board, const GameState& state) {
    std::vector<int> array;
    array.reserve(29);
    
    // Board cells
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            array.push_back(encode_cell(board.at(x, y)));
        }
    }
    
    // Tile inventory
    const auto& black_inv = state.inventory(Player::Black);
    const auto& white_inv = state.inventory(Player::White);
    array.push_back(black_inv.black);
    array.push_back(black_inv.gray);
    array.push_back(white_inv.black);
    array.push_back(white_inv.gray);
    
    return array;
}
```

### Python Example

```python
def encode_cell(occupant, tile):
    """
    occupant: 0=None, 1=Black, 2=White
    tile: 0=None, 1=Black, 2=Gray
    """
    return occupant * 3 + tile

def decode_cell(value):
    """Returns (occupant, tile)"""
    if not 0 <= value <= 8:
        raise ValueError(f"Invalid cell value: {value}")
    return (value // 3, value % 3)

def board_to_array(board, tile_inventory):
    """
    board: 5x5 nested list [[cell00, cell01, ...], [cell10, ...], ...]
    tile_inventory: dict with 'black' and 'white' player inventories
    Returns: flat list of 29 encoded values
    """
    array = []
    
    # Board cells (25 elements)
    for y in range(5):
        for x in range(5):
            cell = board[y][x]
            array.append(encode_cell(cell['occupant'], cell['tile']))
    
    # Tile inventory (4 elements)
    black_inv = tile_inventory['black']
    white_inv = tile_inventory['white']
    array.append(black_inv['black_tiles'])  # [25]
    array.append(black_inv['gray_tiles'])   # [26]
    array.append(white_inv['black_tiles'])  # [27]
    array.append(white_inv['gray_tiles'])   # [28]
    
    return array

def array_to_board(array):
    """
    array: list of 29 encoded values
    Returns: (board, tile_inventory)
    """
    if len(array) != 29:
        raise ValueError("Array must have 29 elements")
    
    # Decode board
    board = []
    for y in range(5):
        row = []
        for x in range(5):
            idx = y * 5 + x
            occupant, tile = decode_cell(array[idx])
            row.append({'occupant': occupant, 'tile': tile})
        board.append(row)
    
    # Decode tile inventory
    tile_inventory = {
        'black': {
            'black_tiles': array[25],
            'gray_tiles': array[26]
        },
        'white': {
            'black_tiles': array[27],
            'gray_tiles': array[28]
        }
    }
    
    return board, tile_inventory
```

### JavaScript Example

```javascript
function encodeCell(occupant, tile) {
    // occupant: 0=None, 1=Black, 2=White
    // tile: 0=None, 1=Black, 2=Gray
    return occupant * 3 + tile;
}

function decodeCell(value) {
    if (value < 0 || value > 8) {
        throw new Error(`Invalid cell value: ${value}`);
    }
    return {
        occupant: Math.floor(value / 3),
        tile: value % 3
    };
}

function boardToArray(board, tileInventory) {
    // board: 5x5 array
    // tileInventory: { black: {...}, white: {...} }
    const array = [];
    
    // Board cells (25 elements)
    for (let y = 0; y < 5; y++) {
        for (let x = 0; x < 5; x++) {
            const cell = board[y][x];
            array.push(encodeCell(cell.occupant, cell.tile));
        }
    }
    
    // Tile inventory (4 elements)
    array.push(tileInventory.black.blackTiles);  // [25]
    array.push(tileInventory.black.grayTiles);   // [26]
    array.push(tileInventory.white.blackTiles);  // [27]
    array.push(tileInventory.white.grayTiles);   // [28]
    
    return array;
}

function arrayToBoard(array) {
    if (array.length !== 29) {
        throw new Error("Array must have 29 elements");
    }
    
    // Decode board
    const board = [];
    for (let y = 0; y < 5; y++) {
        const row = [];
        for (let x = 0; x < 5; x++) {
            const idx = y * 5 + x;
            const cell = decodeCell(array[idx]);
            row.push(cell);
        }
        board.push(row);
    }
    
    // Decode tile inventory
    const tileInventory = {
        black: {
            blackTiles: array[25],
            grayTiles: array[26]
        },
        white: {
            blackTiles: array[27],
            grayTiles: array[28]
        }
    };
    
    return { board, tileInventory };
}
```

## Initial Board State Example

初期盤面の配列表現:

```
     a   b   c   d   e
  5  x   x   x   x   x    (Black pieces)
  4  .   .   .   .   .
  3  .   .   .   .   .
  2  .   .   .   .   .
  1  o   o   o   o   o    (White pieces)
  
  Tile Inventory:
    Black: 3 black tiles, 1 gray tile
    White: 3 black tiles, 1 gray tile
```

配列 (29要素):
```json
[
  3,3,3,3,3,  // rank 5: black pieces
  0,0,0,0,0,  // rank 4: empty
  0,0,0,0,0,  // rank 3: empty
  0,0,0,0,0,  // rank 2: empty
  6,6,6,6,6,  // rank 1: white pieces
  3,1,3,1     // tile inventory: [black_b, black_g, white_b, white_g]
]
```

解説:
- インデックス 0-4 (rank 5): 値3 = Black piece, no tile
- インデックス 5-19 (ranks 4-2): 値0 = empty, no tile
- インデックス 20-24 (rank 1): 値6 = White piece, no tile
- インデックス 25: 3 = 黒プレイヤーの黒タイル残数
- インデックス 26: 1 = 黒プレイヤーの灰色タイル残数
- インデックス 27: 3 = 白プレイヤーの黒タイル残数
- インデックス 28: 1 = 白プレイヤーの灰色タイル残数

## Endgame Example (Tiles Depleted)

終盤でタイルを使い切った例:

```
     a   b   c   d   e
  5  x   .   .   .   .
  4  .   x   ( ) .   .    (gray tile at c4)
  3  .   [ ] x   .   .    (black tile at b3)
  2  .   .   .   o   .
  1  .   .   .   o   o
  
  Tile Inventory:
    Black: 0 black tiles, 0 gray tiles (depleted)
    White: 2 black tiles, 1 gray tile (still available)
```

配列:
```json
[
  3,0,0,0,0,  // rank 5
  0,3,2,0,0,  // rank 4: cell[2]=gray tile (empty+gray=0*3+2=2)
  0,1,3,0,0,  // rank 3: cell[1]=black tile (empty+black=0*3+1=1)
  0,0,0,6,0,  // rank 2
  0,0,0,6,6,  // rank 1
  0,0,2,1     // tiles: black depleted, white has 2 black + 1 gray
]
```

**戦略的意味**: 
- 黒プレイヤーはもうタイルを置けない（終盤の移動のみ）
- 白プレイヤーはまだ3枚のタイルで盤面を有利に変更可能
- この情報はAIの評価関数に必須

## Compatibility

このエンコーディング（v2.0）は以下と互換性があります:

- ✅ Web API エンドポイント - `/api/game/:id/board_array`
- ✅ セル部分（0-24）は従来のNTuple AIと同じエンコーディング
- ⚠️ **完全な29要素形式は新しいフォーマット** - 既存のNTuple実装は25要素のみ使用

### Migration from v1.0 (25 elements)

v1.0（25要素）からv2.0（29要素）への移行:

```python
# v1.0配列（25要素）をv2.0（29要素）に変換
def migrate_v1_to_v2(v1_array):
    if len(v1_array) != 25:
        raise ValueError("v1 array must be 25 elements")
    
    v2_array = v1_array.copy()
    # デフォルトのタイル残数を追加（初期値）
    v2_array.extend([3, 1, 3, 1])  # black_b, black_g, white_b, white_g
    return v2_array
```

### Future NTuple Integration

NTupleネットワークを29要素形式で学習する場合の考慮事項:

1. **特徴抽出**: タイル残数を追加の特徴量として組み込む
2. **パターン設計**: 盤面パターン + タイル情報の組み合わせ
3. **状態空間**: 9^25 × 4 × 2 × 4 × 2 = 約6.2京 通りの状態
4. **メモリ最適化**: 全状態を保存せず、重要な部分空間のみ学習

詳細は別途設計ドキュメント参照。

## Version History

- **v2.0** (2025-11-16): タイル残数を含む29要素フォーマットに拡張
- **v1.0** (2025-11-15): 盤面のみ25要素フォーマット（初期版）
