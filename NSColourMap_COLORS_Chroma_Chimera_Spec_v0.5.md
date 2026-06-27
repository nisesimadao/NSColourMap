# NSColourMap 仕様書 v0.5

> **ClaudeCode / Codex 実装用仕様書**  
> Project: **NSColourMap**  
> Direction: **PITCHMAP::COLORS × Xynth Audio Chroma のキメラ**  
> Goal: **Colour Bass専用・手軽なスペクトラルカラー変換VST**  
> Framework: JUCE + CMake + C++17  
> Status: Legacy-free redesign / MVP-first spec

---

## 0. 方針の完全リセット

この仕様書は、過去のNSColourMap仕様の「Resonator Bank中心」「Scale Resonance中心」「HiTECH汎用対応中心」の考え方をいったん破棄し、**Colour Bass専用の新しいVST**として再設計する。

ただし、過去仕様の良い教訓だけは抽出する。

- Sub Protectは必要
- Transient保持は必要
- MIDI入力は重要
- DAWルーティング確認用のMIDI LEDは必要
- ClaudeCodeが巨大DSPへ暴走しない制約は必要
- UIは固定座標ではなくレスポンシブにする

今回のNSColourMapは、以下の2つを混ぜたキメラとして設計する。

```txt
PITCHMAP::COLORS 的な要素:
- pitch grid
- MIDI keyboard / UI keyboardでターゲット音を作る
- audioをtonalityへ寄せる
- electronic sound design向けの強い変形
- scale shift
- formant shift / formant gamma
- transient bypass
- high-pass / low-pass
- 3系統以上のcharacter modes

Xynth Audio Chroma 的な要素:
- とにかく手軽
- 少ないノブ
- key / scaleを選んで即効く
- Colorノブ中心
- 0 latency / high quality切替
- visualizerでDRY/TUNEDが分かる
- affected frequency rangeを簡単に調整
- side-bands mute
- spectral morph / spectral gate
- polyphonic sampleにも使える体感
```

---

## 1. 一文コンセプト

**NSColourMapは、PITCHMAP::COLORSの派手なピッチグリッド変換感と、Xynth Audio Chromaの手軽な“挿してKey/Scale/Colorを回すだけ”の操作感を融合した、Colour Bass専用スペクトラルカラー変換VSTである。**

---

## 2. 製品思想

### 2.1 何を作るか

NSColourMapは、ベース、ノイズ、FM、ワブル、ボーカルチョップ、メタリック素材を、曲のキーやMIDIコードに合ったColour Bass素材へ変換する。

ユーザーが最初にやることはこれだけでよい。

```txt
1. 音素材トラックにNSColourMapを挿す
2. Key / Scaleを選ぶ、またはMIDIコードを送る
3. COLORを上げる
4. 必要ならMODEとSHIFTを触る
5. Mix / Sub Protectで馴染ませる
```

### 2.2 何を作らないか

MVPでは以下を作らない。

- 完全なPITCHMAP::COLORSの内部アルゴリズム再現
- 既存製品のUIアセットコピー
- 既存製品のモード名コピー
- 汎用ピッチ補正ソフト
- Melodyne / Auto-Tune系の音程補正
- AI音源分離
- DAW代替
- 巨大マルチエフェクト

ただし、**PITCHMAP::COLORSにめちゃくちゃ寄せた体験**は目指す。

つまり、コピーするのは名称やUIではなく、次の体験である。

```txt
入力音がピッチグリッドへ吸い寄せられ、
電子的でカラフルな倍音へ変わり、
MIDIやScale Shiftで音色が音楽的に動く体験。
```

---

## 3. 参考製品から取り込む要素

### 3.1 PITCHMAP::COLORSから取り込む要素

| 要素 | NSColourMapでの解釈 | 優先度 |
|---|---|---|
| Pitch Grid | ターゲット音の集合。MIDI/Scale/UI Keyboardで作る | MVP |
| MIDI Keyboard Grid | MIDIコードでターゲット音を決める | MVP |
| UI Keyboard Grid | UI上の鍵盤でKey/Scale/Rootを決める | MVP |
| Electronic character modes | Clean / Color / Hyper / Alien / Glitch | MVP |
| Scale Shift | Pitch Gridを半音単位でずらす | MVP |
| Formant Shift | 疑似フォルマント/スペクトル包絡移動 | MVP〜v1 |
| Formant Gamma | フォルマント山谷の誇張 | v1 |
| Transient Bypass | アタックを処理から逃がして戻す | MVP |
| HPF / LPF | 処理対象帯域の制限 | MVP |
| Sound design first | 自然補正より派手さ優先 | MVP |

### 3.2 Chromaから取り込む要素

| 要素 | NSColourMapでの解釈 | 優先度 |
|---|---|---|
| Harmonic Sweetener | 破壊より“良い感じに色を足す”基本体験 | MVP |
| Snap audio to scale | Key/Scale指定で即効くScale Grid | MVP |
| Color knob中心 | COLORを巨大ノブにする | MVP |
| COLOR > 100の追加色付け | 100%以降でDecay Tail / Resonanceを足す | MVP |
| 0 Latency / High Quality | Mode toggleとして用意 | MVP |
| Fast Workflow | UIは少ない操作で成立 | MVP |
| Clear Sound | tail reduction / transient preservation | MVP |
| Visualizer | DRY/TUNEDの違いが見える表示 | MVP |
| Affected Frequency Range | 処理帯域をドラッグ/ノブで指定 | MVP |
| Side-band mute | grid外/side成分を抑える | v1 |
| Spectral Morph | wetの色を保ちつつdryの輪郭を戻す | v1 |
| Spectral Gate | tail/ringing短縮 | v1 |
| Multirate | CPU節約モード | v1 |

---

## 4. MVPの正体

MVPは、以下の4レイヤーで構成する。

```txt
Audio In
↓
1. Frequency Region Selector
   - HPF / LPF / Sub Protect
   - Affected range
↓
2. Pitch Grid Target Generator
   - Key / Scale
   - MIDI Chord
   - Scale Shift
↓
3. Colour Mapping Engine
   - MVPではResonator/Spectral-ish Harmonic Mapperで近似
   - v2でSTFT Pitch-Class Mapperへ拡張
↓
4. Clarity / Chroma Controls
   - COLOR
   - Transient Bypass
   - Formant
   - Gamma
   - Morph / Gate later
↓
Mix / Output
↓
Audio Out
```

重要:

```txt
MVPでは、PITCHMAP::COLORS的な“個別ピッチ分離”は完全再現しない。
ただしUIと操作体験はPitch Grid Colour Mappingとして設計する。
内部実装は、最初は軽量なHarmonic Mapperでよい。
```

---

## 5. UIコンセプト

### 5.1 基本思想

NSColourMapのUIは、PITCHMAP::COLORSの「ピッチグリッドで音を染める」感と、Chromaの「少ない操作で即効く」感を両立する。

画面の中心は **Visualizer + Pitch Grid Keyboard + Big COLOR knob** である。

### 5.2 レイアウト

```txt
┌────────────────────────────────────────────────────┐
│ NSColourMap | Preset | MIDI ● | 0 Lat / HQ | Bypass│
├────────────────────────────────────────────────────┤
│ Key / Scale / MIDI Grid / Scale Shift              │
├────────────────────────────────────────────────────┤
│                                                    │
│             DRY / TUNED Visualizer                 │
│     pitch lanes / affected range / sub zone         │
│                                                    │
├────────────────────────────────────────────────────┤
│ UI Keyboard / Active MIDI Notes / Target Grid       │
├────────────────────────────────────────────────────┤
│        BIG COLOR KNOB                               │
│ Amount | Formant | Gamma | Transient | Mix          │
├────────────────────────────────────────────────────┤
│ Mode: Clean / Color / Hyper / Alien / Glitch        │
│ Range: Low Cut / High Cut / Side Mute / Morph / Gate│
└────────────────────────────────────────────────────┘
```

### 5.3 Chroma的な手軽さのためのルール

UIは以下を守る。

- 最初に触るノブは **COLOR**
- Key/Scaleは1クリックで選べる
- MIDIが来たかすぐ分かる
- 0 Latency / HQはワンタップ切替
- Visualizerは「見て気持ちいい」より「今何が変わっているか」を優先
- Advancedは折りたたむ
- 1画面で音作り開始できる

---

## 6. Visualizer仕様

### 6.1 役割

Visualizerは、Chromaの分かりやすさを強く参考にする。

MVPで表示するもの:

```txt
- 入力の周波数エネルギー
- DRY成分
- TUNED / COLORED成分
- Pitch Gridのターゲットレーン
- Sub Protect領域
- Affected Frequency Range
- MIDI active notes
- Scale active notes
```

### 6.2 色の意味

```txt
DRY:
- 原音またはほぼ変更されていない成分
- 紫 / 暗い色系

TUNED:
- pitch gridへ移動・強調された成分
- 青 / 水色系

COLORED:
- COLOR 100%以上で追加された共鳴/尾/倍音
- 黄 / オレンジ / ピンク系

PROTECTED:
- Sub Protectまたは処理対象外
- 暗いグレー
```

### 6.3 周波数範囲

MVPのVisualizerは、Chroma的に中高域を見やすくする。

```txt
Default visible range: 100Hz〜6kHz
Expanded range: 40Hz〜12kHz
```

### 6.4 Affected Frequency Range

ユーザーは処理対象帯域を指定できる。

```txt
Low Cut: 40Hz〜500Hz
High Cut: 1kHz〜16kHz
```

`Low Cut` は Sub Protect と連動してよい。

MVPではノブ指定でよい。  
v1以降でVisualizer上のドラッグ操作に対応する。

### 6.5 注意

MVPのVisualizerは診断表示であり、編集グリッドではない。

MVPでやらないこと:

- 周波数点の直接ドラッグ
- 本格PitchMap風ノート編集
- 完全なスペクトルピッチ分離表示

---

## 7. Pitch Grid仕様

### 7.1 Pitch Gridとは

Pitch Gridは、NSColourMapが音を寄せる/強調するターゲット音の集合である。

Pitch Gridは以下から作る。

```txt
1. Key / Scale
2. MIDI Chord
3. UI Keyboard
4. Scale Shift
```

MVPで必須:

- Key / Scale Grid
- MIDI Grid
- Scale Shift

v1で追加:

- UI Keyboardによる手動一時追加
- MIDI + Scaleの融合表示

v2で追加:

- Custom Grid Editor
- ノート単位のmute / weight

---

## 8. 操作モード

### 8.1 Scale Grid Mode

Chroma的な標準モード。

```txt
Key + Scaleを選ぶ
↓
入力音の中高域がそのスケール感に染まる
↓
COLORで濃さを調整
```

MIDIなしで使える。  
最も手軽なモードとして、初回起動時はこのモードでもよい。

### 8.2 MIDI Grid Mode

PITCHMAP::COLORS的なモード。

```txt
MIDIコードを送る
↓
押されているノートを全オクターブへ展開
↓
音素材がそのコード感に染まる
```

MIDIがない場合は最後のコードを保持する。

### 8.3 Hybrid Grid Mode

優先度: v1

```txt
Scale Gridを土台にする
MIDIで押した音を強調する
```

Colour Bassでは、曲のキーから外れずにMIDIコードで動かせるので便利。

### 8.4 UI Grid Mode

優先度: v1〜v2

画面上の鍵盤でターゲット音を選ぶ。  
MVPではKey/Scale選択用の鍵盤表示だけでよい。

---

## 9. メインパラメータ

### 9.1 Big COLOR

最重要ノブ。

```txt
COLOR: 0〜200%
```

動作:

```txt
0〜100%:
- dry/wet的にcolour mapping量を増やす
- Chroma的な“手軽に色を足す”範囲

100〜200%:
- additional resonance
- colourful decay tail
- stronger harmonic density
- more electronic flavour
```

注意:

- 100%を超えたら急に音量暴走しないようにする
- tailが伸びるのでGate / Transient / Morphと相性を持たせる

### 9.2 Amount

```txt
Amount: 0〜100%
```

Pitch Gridへ寄せる/強調する強さ。  
COLORより技術寄りの調整。

Chroma的な手軽さを優先するなら、ユーザーは基本COLORだけ触っても成立する。

### 9.3 Scale Shift

```txt
Scale Shift: -12〜+12 semitones
```

Pitch Grid全体を半音単位で移動する。  
オートメーション前提。

用途:

- drop中に音色を動かす
- 同じ素材から別コード感を作る
- HiTECHフィルで高速に動かす

### 9.4 Formant

```txt
Formant: -24〜+24 semitones
```

PITCHMAP::COLORS的なサイズ感・声感を作る。

MVPでは本物ではなく疑似フォルマントでよい。

### 9.5 Gamma

```txt
Gamma: 0〜100%
```

フォルマントやスペクトル山谷の誇張。  
PITCHMAP::COLORSのFormant Gamma的な“説明しにくい有機的変形”を目指す。

MVPでは非表示でもよい。  
v1で追加する。

### 9.6 Transient

```txt
Transient: 0〜150%
```

アタックを処理から逃がして戻す。

```txt
0%: 戻さない
50〜70%: 標準
100%以上: アタック強調
```

### 9.7 Morph

優先度: v1

ChromaのSpectral Morph的な役割。

```txt
Morph: 0〜100%
```

意味:

```txt
0%:
- wet処理そのまま

100%:
- wetの色は保ちながらdryの輪郭/強弱に近づける
```

MVPでは、時間領域の簡易Envelope Morphでよい。  
v2でスペクトルMagnitude Morphへ拡張。

### 9.8 Gate

優先度: v1

ChromaのSpectral Gate的な役割。

```txt
Gate: 0〜100%
```

役割:

- ringing / tail を短くする
- COLOR 100%以上のDecayを制御する
- HiTECHで短いレーザー感を作る

### 9.9 Low Cut / High Cut

```txt
Low Cut: 40Hz〜500Hz
High Cut: 1kHz〜16kHz
```

処理対象周波数範囲を指定する。

### 9.10 Mix / Output

```txt
Mix: 0〜100%
Output: -12〜+12dB
```

---

## 10. Character Modes

既存製品のモード名はコピーしない。  
ただし、PITCHMAP::COLORSの「自然寄り / 共鳴寄り / 電子的破壊寄り」の3段階思想をかなり参考にする。

### 10.1 Clean

Chroma寄り。  
破壊せず、曲のキーに馴染ませる。

```txt
COLOR response: smooth
resonance: low
transient: high
tail: short
artifacts: low
```

### 10.2 Color

標準。  
Colour Bassの基本。

```txt
COLOR response: strong
resonance: medium
transient: medium
tail: medium
artifacts: controlled
```

### 10.3 Hyper

PITCHMAP::COLORS寄り。  
派手、明るい、強い共鳴。

```txt
COLOR response: very strong
resonance: high
gamma: medium-high
tail: medium-long
high emphasis: strong
```

### 10.4 Alien

PITCHMAP::COLORSの電気的・異質な方向を独自名で表現。

```txt
formant: reactive
gamma: high
scale shift automation: encouraged
texture: organic / electric / strange
```

### 10.5 Glitch

HiTECH / fills / lasers向け。

```txt
glide: short
transient: high
gate: high
tail: short
colour: sharp
```

---

## 11. Quality Modes

Chroma的な手軽さとして、品質切替は分かりやすくする。

### 11.1 0 Latency

```txt
Latency: 0 samples target
CPU: low
Artifacts: more
Use: live preview / composition / low-latency work
```

MVPではこのモードを優先。

### 11.2 High Quality

```txt
Latency: allowed, e.g. 24ms〜
CPU: higher
Artifacts: fewer
Use: final render / cleaner output
```

MVPではボタンだけ用意し、内部挙動は軽い違いでもよい。  
v1でFFTサイズやoversampling、smoother settingsを変える。

### 11.3 Multirate

優先度: v1

CPU削減用。

```txt
Multirate On:
- internal lower sample-rate processing
- lower CPU
- slightly different tone
```

---

## 12. DSP設計

### 12.1 MVP信号フロー

```txt
Audio In
↓
Input Safety / DC Block
↓
Affected Range Split
├─ Protected Low / Out-of-range path
└─ Active Processing Range
    ↓
    Pitch Grid Target Generator
    ↓
    Colour Mapping Core
    ↓
    Character Mode Shaper
    ↓
    Pseudo Formant / Gamma
    ↓
    Transient Restore
    ↓
    Tail / Gate / Morph Control
↓
Mix
↓
Output Safety
↓
Audio Out
```

### 12.2 Colour Mapping Core MVP

PITCHMAP::COLORS的な体験を目指すが、MVPでは軽量近似にする。

内部候補:

```txt
- pitch-grid tuned resonator layer
- harmonic emphasis filters
- spectral-ish peak emphasis
- dry/wet magnitude-like envelope blending
```

必須要件:

- Key/Scaleで音が変わる
- MIDIコードで音が変わる
- COLORで派手になる
- Scale Shiftで明確に音が動く
- Sub/low rangeが崩れにくい

### 12.3 v2 Colour Mapping Core

v2ではよりPITCHMAP::COLORSへ寄せる。

候補:

```txt
- STFT / CQT analysis
- pitch-class bin grouping
- target grid frequency relocation
- magnitude morph
- spectral gate per bin
- transient-aware wet/dry recombination
```

ただし、MVPでは実装しない。

---

## 13. パラメータ一覧

| ID | 表示名 | 範囲 | 初期値 | 優先度 |
|---|---|---:|---:|---|
| mode | Grid Mode | Scale / MIDI / Hybrid / UI | Scale | MVP |
| character | Character | Clean / Color / Hyper / Alien / Glitch | Color | MVP |
| color | COLOR | 0〜200% | 65% | MVP |
| amount | Amount | 0〜100% | 70% | MVP |
| scaleShift | Scale Shift | -12〜+12 st | 0 st | MVP |
| formant | Formant | -24〜+24 st | 0 st | MVP |
| gamma | Gamma | 0〜100% | 0% | v1 |
| transient | Transient | 0〜150% | 65% | MVP |
| morph | Morph | 0〜100% | 0% | v1 |
| gate | Gate | 0〜100% | 0% | v1 |
| lowCut | Low Cut | 40〜500Hz | 100Hz | MVP |
| highCut | High Cut | 1k〜16kHz | 6000Hz | MVP |
| sideMute | Side Mute | Off / On | Off | v1 |
| key | Key | C〜B | C | MVP |
| scale | Scale | enum | Minor Pentatonic | MVP |
| midiFreeze | Freeze | Off / On | On | MVP |
| quality | Quality | 0 Latency / High Quality | 0 Latency | MVP |
| mix | Mix | 0〜100% | 65% | MVP |
| output | Output | -12〜+12dB | 0dB | MVP |
| multirate | Multirate | Off / On | Off | v1 |

---

## 14. Key / Scale仕様

### 14.1 Root Note

```txt
C / C# / D / D# / E / F / F# / G / G# / A / A# / B
```

UI:

- dropdown
- mini keyboard click
- arrows / mouse wheel

### 14.2 Scales

MVP:

```txt
Major
Natural Minor
Harmonic Minor
Dorian
Phrygian
Lydian
Mixolydian
Minor Pentatonic
Major Pentatonic
Pentatonic Blues
Whole Tone
Chromatic
```

v1:

```txt
Custom scales via JSON
Relative major/minor quick switch
Favourite scales
```

---

## 15. MIDI仕様

### 15.1 MIDI Grid

MIDI Note Onでターゲット音を追加する。

```txt
MIDI C3 E3 G3
↓
C/E/G pitch classes are enabled across all octaves
```

### 15.2 Freeze

MIDIが止まっても最後のコードを保持する。

初期値:

```txt
Freeze: On
```

### 15.3 MIDI LED

MIDI入力があればHeaderのLEDを点灯する。

### 15.4 MIDI routing help

READMEに必ず書く。

```txt
If the plugin does not react:
1. Check MIDI LED
2. Enable MIDI Grid mode
3. Turn Freeze on
4. Try Scale Grid mode first
5. Increase COLOR and Mix
```

---

## 16. UI詳細

### 16.1 Header

```txt
NSColourMap | Preset | MIDI ● | 0 Lat / HQ | Bypass | Settings
```

### 16.2 Key Strip

```txt
Grid Mode: Scale / MIDI / Hybrid
Key: C
Scale: Minor Pentatonic
Scale Shift: 0
```

### 16.3 Visualizer

Chroma的に、中央に大きく表示する。

表示:

- DRY / TUNED / COLORED
- affected range
- protected low range
- target lanes
- input energy

### 16.4 Keyboard

UI KeyboardはPITCHMAP::COLORS的に重要。

MVP:

- selected scale notes highlight
- MIDI active notes highlight
- root note highlight

v1:

- click to set root
- click notes for temporary UI grid

v2:

- custom grid editing

### 16.5 Big COLOR Section

中央下に大きく置く。

```txt
        COLOR
       [  65%  ]
```

COLORの周囲に小さく以下を置く。

```txt
Amount
Formant
Transient
Mix
```

Advanced drawer:

```txt
Gamma
Morph
Gate
Low Cut
High Cut
Side Mute
Multirate
```

---

## 17. プリセット

MVPでは少数でよい。

```txt
01 Chroma Bass Start
02 Clean Scale Snap
03 MIDI Colour Chord
04 Hyper Grid Bass
05 Alien Formant Sweep
06 Glitch Laser Fill
07 Noise To Key
08 Vocal Chop Color
09 Sub Safe Growl
10 COLOR 150 Tail
```

方針:

- 有名製品名・アーティスト名を使わない
- 用途がすぐ分かる名前
- MIDI必須プリセットは名前にMIDIを入れる
- COLOR > 100系はtail/ringingの説明を付ける

---

## 18. ClaudeCode向け実装順

### Phase 0: Legacy-free Setup

- 新仕様としてREADME/AGENTS/docsを作る
- 過去仕様のResonator-only表現を消す
- ただしSub ProtectやTransientなど必要な概念は残す

### Phase 1: Minimal JUCE Plugin

- VST3 Audio Effect
- MIDI input enabled
- stereo in/out
- APVTS
- empty UI

### Phase 2: Key / Scale / MIDI Grid

- ScaleNoteSet
- PitchGridState
- MidiGridState
- Scale Shift
- Freeze
- MIDI LED

### Phase 3: Affected Range + Safety

- Low Cut / High Cut
- Sub Protect
- DC Block
- Output safety

### Phase 4: Colour Mapping Core MVP

- grid-tuned harmonic mapper
- resonator/peak emphasis layer
- COLOR 0〜200 behavior
- Amount / Mix

### Phase 5: Visualizer MVP

- DRY/TUNED/COLORED diagnostic display
- target lanes
- affected range
- keyboard highlight

### Phase 6: Character Modes

- Clean
- Color
- Hyper
- Alien
- Glitch

### Phase 7: Pseudo Formant + Transient

- Formant
- Transient Bypass
- basic Gamma placeholder

### Phase 8: Chroma-like Advanced Tools

- Morph
- Gate
- Side Mute
- Multirate
- Custom scales JSON

### Phase 9: v2 Spectral Mapper

- STFT/CQT pitch-class mapper
- magnitude morph
- spectral gate per bin
- custom grid editor

---

## 19. AGENTS.md用ルール

```md
# AGENTS.md

## Project Goal
Build NSColourMap: a Colour Bass focused VST inspired by PITCHMAP::COLORS-style pitch-grid colour mapping and Chroma-style fast workflow.

## Core UX
The plugin should feel like:
- choose a key/scale or send MIDI chords
- turn COLOR
- instantly hear colourful, in-key harmonic transformation

## Important
This is not the old resonator-only NSColourMap spec.
Do not preserve legacy architecture unless it supports the new COLORS + Chroma direction.

## MVP
The MVP must prove:
1. Key/Scale changes the harmonic colour.
2. MIDI chords change the pitch grid.
3. COLOR 0-200 changes dry/wet and extra resonance/tail.
4. Visualizer shows DRY/TUNED/COLORED feedback.
5. Low-end protection prevents bass mud.

## Do Not
- Do not copy UI assets, names, or branding from commercial plugins.
- Do not implement full PITCHMAP/COLORS internally in MVP.
- Do not build a huge UI before the audio core works.
- Do not keep old terminology like Scale Resonance as the main UX label.
- Do not make HiTECH a separate DSP engine.
- Do not block the build on advanced spectral code.

## Implementation Rules
- Keep changes small and buildable.
- Preserve UTF-8.
- Audio thread must not allocate, lock, log, or do file I/O.
- UI and DSP must be separated.
- Every DSP class should be testable without the UI.
- Prefer simple MVP approximations behind stable interfaces.
```

---

## 20. 完了条件

MVP完了条件:

- VST3としてDAWで開ける
- Key/Scaleを選ぶと音の色が変わる
- MIDIコードを送るとPitch Gridが変わる
- COLORノブだけでもColour Bass的な変化が出る
- COLOR 100%以上でtail/resonanceが増える
- Low Cut / Sub Protectで低域が安定する
- Transientでアタックを戻せる
- 0 Latency / HQ切替がある
- VisualizerにDRY/TUNED/COLOREDの違いが出る
- UI Keyboardにroot/scale/MIDI noteが表示される
- Mix/Outputで扱いやすい音量にできる
- READMEに使い方とMIDIルーティングがある

---

## 21. 一文まとめ

**NSColourMap v0.5は、PITCHMAP::COLORSのピッチグリッドによる電子的な音色変換感と、Xynth Audio Chromaの少ない操作でKey/Scaleへスナップする手軽さを融合した、Colour Bass専用のスペクトラルカラー変換VSTである。**

過去のResonator-only仕様は引き継がない。  
ただし、MVPの内部実装では、Pitch Grid Colour Mappingを軽量に近似するために、Resonator / Harmonic Mapper / Spectral-ish Processingを段階的に使ってよい。
