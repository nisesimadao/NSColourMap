# NSColourMap 仕様書 v0.3

> **ClaudeCode / Codex 実装用仕様書**  
> Project: **NSColourMap**  
> Goal: Colour Bass / HiTECH 向け MIDI・スケール連動ハーモニックカラー変換 VST  
> Framework: JUCE + CMake + C++17  
> Core: MIDI/Scale-aware Resonator Bank + Colour Processor + Sub Protect + Transient Bypass

---

## 0. 最重要方針

NSColourMap のMVPは、**本物のPitchMapクローンでも、ポリフォニック・ピッチ補正エンジンでもない**。

MVPの正体は以下。

```txt
MIDI / Scale-aware Resonator Effect
+
Colour Processor
+
Sub Protect
+
Transient Bypass
+
Pseudo Formant Tone Shaper
```

最初に証明すべきユースケースはこれ。

```txt
1. ベース / ノイズ / ボーカルチョップのトラックにNSColourMapを挿す
2. MIDIコードをNSColourMapへ送る
3. コードに合わせて共鳴・倍音・色が変わる
4. Sub Protectで低域が安定する
5. Mixで原音と処理音をブレンドできる
```

このユースケースが成立していない段階で、以下を実装してはいけない。

- 本格STFT再合成
- Custom Map Editor
- 高度なSpectrum Mapアニメーション
- 大量プリセット
- 独立したHiTECH専用エンジン
- 本物のフォルマントシフター
- AI音源分離
- PitchMap風のUIコピー

---

## 1. 優先度タグ

機能は削除しない。代わりに、実装優先度を明確に分ける。

| タグ | 意味 |
|---|---|
| `MVP` | 最初の実用品に必須。ここが動くまで他へ進まない |
| `v1` | MVP後に追加する実用強化 |
| `v1.5` | 余裕があれば追加する音作り強化 |
| `v2` | 本格拡張。重いDSPや高度UIを含む |
| `Experimental` | 実験枠。MVPを邪魔しない範囲でのみ扱う |

---

## 2. プロジェクト名

**NSColourMap**

読み方は以下のどちらでもよい。

```txt
NS Color Map
NS Colour Map
```

UI表示は `NSColourMap` で統一する。

---

## 3. 一文コンセプト

**NSColourMapは、MIDIコードやスケールに合わせて、ベース・ノイズ・シンセ・ボーカルチョップ素材を、Colour Bass / HiTECH向けの倍音テクスチャへ変換する、MIDI入力前提の無料VSTである。**

---

## 4. ClaudeCodeへの最初の指示

このプロジェクトは、**Colour Bass / HiTECH制作向けのリアルタイム・ハーモニックカラー変換VST**を作るプロジェクトです。

目的は、PitchMap系、Vocoder系、Resonator / Spectral Resonator系の制作ワークフローを参考にしつつ、**MIDIコード入力とスケール指定を前提**に、ベース・ノイズ・シンセ・ボーカルチョップ素材をコード感のあるColour Bass / HiTECHテクスチャへ変換する無料VSTを実装することです。

ただし、最初からPitchMapの内部アルゴリズムを再現しようとしてはいけません。

MVPでは、以下を作ってください。

```txt
MIDI Chord Mode
Scale Resonance Mode
Sub Protect
Transient Bypass
MIDI-controlled Resonator Bank
Colour Processor
Pseudo Formant Tone Shaper
Dry/Wet Mix
Responsive JUCE UI
```

### 絶対にやらないこと

ClaudeCodeは、MVP段階で以下を実装してはいけません。

- 完全なPitchMapクローン
- 本格的なポリフォニック音源分離
- STFT再合成ベースの巨大DSP
- AI / 機械学習による音源分離
- Audio-to-MIDI解析
- 本格Auto-Tune / Melodyne系ピッチ補正
- 巨大なUI実装
- 100個以上のプリセット
- Web UI化
- 独立したHiTECH専用DSPエンジン
- 本物の位相ボコーダー式フォルマントシフター

---

## 5. 参考にする制作ワークフロー

### 5.1 Vocoder型 Colour Bass

よくある手法:

```txt
Bass / Noise / Vocal Chop を素材にする
↓
MIDIコードやPadでコード感を与える
↓
Vocoderで音階感を作る
↓
OTT / Saturation / EQ / Reverbで派手にする
```

NSColourMapへの反映:

| 反映内容 | 優先度 |
|---|---|
| MIDI入力を前提にする | MVP |
| MIDI LEDを入れる | MVP |
| DAW別ルーティングをREADMEに書く | MVP |
| VocoderそのものではなくResonator/Colour Processorで近い体感を作る | MVP |
| 本格Vocoderモード | v2 |

### 5.2 Resonator型 Colour Bass

よくある手法:

```txt
Bass / Drum / Noise / Metallic Texture
↓
Resonator / Spectral Resonator
↓
MIDI note / chord / scaleで共鳴先を決める
↓
Mixを抑えて原音にコード感を足す
```

NSColourMapへの反映:

| 反映内容 | 優先度 |
|---|---|
| Resonator Bankを中心DSPにする | MVP |
| MIDI Chord / Scaleからターゲット周波数を作る | MVP |
| Voice正規化で音量暴走を防ぐ | MVP |
| Transient Bypassでアタックを戻す | MVP |
| AlgoごとにQ / Decay / Saturation / Widthを変える | MVP〜v1 |

### 5.3 PitchMap型 Colour Bass

よくある手法:

```txt
Bass / Chord / Texture
↓
PitchMap系プラグイン
↓
MIDIやグリッドでターゲット音を指定
↓
音全体をコードやスケールに寄せる
```

NSColourMapへの反映:

| 反映内容 | 優先度 |
|---|---|
| MIDI / Scaleで音色をコード感へ寄せる体感 | MVP |
| Spectrum Mapで処理状態を見せる | MVPでは診断表示 |
| PitchMap風の編集グリッド | v2 |
| STFT pitch-class mapper | v2 |
| Polyphonic pitch correction | Experimental |

### 5.4 HiTECH / Glitch / Psytrance系から拾う要素

HiTECH / Darkpsy / Psytrance系の制作では、以下が重要になりやすい。

- 高速BPMでも崩れない短いトランジェント
- Kick / Bass の低域分離
- グリッチ、短いFM、ノイズ、レーザー、金属音
- 細かいフィル、ピッチラン、フォルマント移動
- Stereo / Haas / Delay的な広がり
- サブはモノ寄りで安定

NSColourMapへの反映:

| 反映内容 | 優先度 |
|---|---|
| HiTECH Algoを追加 | MVP |
| Transient Bypassを必須化 | MVP |
| Formant / Glide / Colourをオートメーション可能にする | MVP |
| HiTECHを高速・短発・硬い挙動にする | MVP |
| Stutter / Gate / Laser専用マクロ | v1 |
| Glitch sequencer | v2 |

重要:

```txt
HiTECHはMVPでは独立DSPエンジンではない。
Clean / Colour / Hyper / HiTECH / Broken は、同じResonatorBank / ColourProcessor / Transient / Glide / Decayを使うチューニングモードである。
```

---

## 6. 非目標

このプラグインは、以下をMVPで目指さない。

| 非目標 | 優先度 |
|---|---|
| 既存製品のUIアセットやブランド表現のコピー | Never |
| 完全なPitchMapアルゴリズム再現 | Experimental |
| ミックス済み音源の完全な個別音分離 | Experimental |
| Audio-to-MIDI変換 | v2以降 |
| ボーカル補正専用Auto-Tune | Never / 別プロジェクト |
| 汎用ピッチ補正ソフト | Never / 別プロジェクト |
| 手動Custom Pitch Mapエディタ | v2 |
| 本格Convolution IRローダー | v2 |
| AI音源分離 | Experimental |
| 内蔵シンセ | v2以降 |
| DAW代替の作曲ツール | Never |

---

## 7. 用語の注意

### 7.1 `Scale Align` という言葉は使わない

旧仕様では `Scale Align` と呼んでいたが、これは誤解を招きやすい。

`Scale Align` と書くと、ClaudeCodeやユーザーが以下を想像しやすい。

```txt
入力音のピッチを検出する
↓
そのピッチをスケール内の音へ補正する
```

しかしMVPでは、それは行わない。

### 7.2 正式名称: `Scale Resonance Mode`

意味:

```txt
キー/スケールからターゲットノート群を生成する
↓
その音に沿ったResonator BankとColour処理を行う
↓
素材の中高域にスケール感・コード感を付ける
```

これは本格ピッチ補正ではなく、**Scale-aware Resonance / Tonal Emphasis** である。

---

## 8. 対応形式

MVPでは以下を目標にする。

```txt
Windows: VST3
macOS: VST3 / AU
Framework: JUCE
Language: C++17以上
Build: CMake
UI: JUCE Componentによる独自描画
Preset/state: AudioProcessorValueTreeState
```

### 8.1 JUCE Plugin Characteristics

CMake / JUCE設定では以下を意識する。

```txt
isSynth = false
wantsMidiInput = true
producesMidiOutput = false
isMidiEffect = false
accepts audio input = true
produces audio output = true
```

このプラグインは、MIDI Effectではなく、**MIDI入力を受け取れるAudio Effect** として作る。

### 8.2 DAWルーティング注意

DAWによって Audio Effect への MIDI ルーティング方法が違う。

READMEには必ず以下を書く。

- Ableton LiveでのMIDIルーティング例
- FL StudioでのMIDIルーティング例
- REAPERでのMIDIルーティング例
- Logic ProでAUを使う場合の注意
- MIDIが来ていないときのScale Resonance Modeの使い方

必要になった場合のみ、以下を検討する。

```txt
NSColourMap FX
- Audio Effect
- MIDI inputあり

NSColourMap Inst
- Instrument互換版
- MIDIルーティングしやすいDAW向け
```

| 形式 | 優先度 |
|---|---|
| VST3 FX | MVP |
| AU FX | v1 |
| Instrument互換版 | v1.5 |
| Standalone | v2 |

---

## 9. ClaudeCode向け開発ルール

### 9.1 最初に作るファイル

```txt
README.md
AGENTS.md
docs/NSColourMap_Spec.md
docs/Research_Notes.md
docs/ClaudeCode_Tasks.md
CMakeLists.txt
Source/
assets/
```

### 9.2 AGENTS.mdに書くべきこと

```md
# AGENTS.md

## Project Goal
Build NSColourMap, a JUCE/CMake VST3/AU plugin for Colour Bass and HiTECH sound design.

## Core Idea
NSColourMap MVP is not a true polyphonic pitch-correction engine.
It is a MIDI/scale-aware harmonic Resonator Bank plus Colour Processor.
The goal is to create the practical Colour Bass feeling of PitchMap/Vocoder/Resonator workflows, not to recreate PitchMap internally.

## MVP Definition
The MVP is a MIDI/scale-aware resonator effect.

It should first prove this simple use case:
1. Insert NSColourMap on a bass/noise/vocal chop track.
2. Send MIDI chords to the plugin.
3. Hear the resonator colour change according to the chord.
4. Keep the sub stable with Sub Protect.
5. Blend the result with Mix.

If this use case does not work yet, do not implement advanced UI, STFT, custom maps, or extra presets.

## Rules
- Keep every change small and buildable.
- Do not implement full PitchMap-style source separation in the MVP.
- Do not implement STFT resynthesis unless explicitly requested in a later version.
- Do not add unrelated features.
- Do not rewrite Japanese documentation unless asked.
- Preserve UTF-8 encoding.
- Prefer simple, testable DSP classes.
- Audio thread must not allocate, lock, log, or perform file I/O.
- UI and DSP must be separated.
- Every DSP class should be usable without the plugin UI.
- If a feature is unclear, implement the smallest useful version first.
- Do not replace the planned architecture with a web UI.
- Do not use absolute layout coordinates for the UI.
- Do not create giant files. Split DSP/UI/parameter code into small classes.
- Do not make hidden global state for audio processing.
- Always keep a working build after each task.
- Do not start Phase 4 or later until Phase 0-3 build and run successfully.
- SpectrumMapView is diagnostic/visual only in the MVP.
- HiTECH is an algorithm tuning mode, not a separate DSP engine in the MVP.
- The MVP Formant control is a pseudo-formant tone shaper, not a true formant shifter.
```

---

## 10. 推奨ディレクトリ構成

```txt
NSColourMap/
├─ README.md
├─ AGENTS.md
├─ CMakeLists.txt
├─ docs/
│  ├─ NSColourMap_Spec.md
│  ├─ Research_Notes.md
│  ├─ ClaudeCode_Tasks.md
│  ├─ Routing_Guide.md
│  └─ DSP_Notes.md
├─ Source/
│  ├─ PluginProcessor.h
│  ├─ PluginProcessor.cpp
│  ├─ PluginEditor.h
│  ├─ PluginEditor.cpp
│  ├─ Parameters.h
│  ├─ Parameters.cpp
│  ├─ State.h
│  ├─ State.cpp
│  ├─ dsp/
│  │  ├─ MidiChordState.h
│  │  ├─ ScaleNoteSet.h
│  │  ├─ TargetNoteGenerator.h
│  │  ├─ SubSplitter.h
│  │  ├─ TransientDetector.h
│  │  ├─ ResonatorVoice.h
│  │  ├─ ResonatorBank.h
│  │  ├─ ColourProcessor.h
│  │  ├─ PseudoFormantTone.h
│  │  ├─ StereoTools.h
│  │  └─ SafetyLimiter.h
│  ├─ ui/
│  │  ├─ Layout.h
│  │  ├─ Theme.h
│  │  ├─ HeaderView.h
│  │  ├─ KeyboardView.h
│  │  ├─ SpectrumMapView.h
│  │  ├─ MacroControlsView.h
│  │  ├─ ModeRowView.h
│  │  └─ SnapshotView.h
│  └─ tests/
│     ├─ ScaleNoteSetTests.cpp
│     ├─ MidiChordStateTests.cpp
│     ├─ TargetNoteGeneratorTests.cpp
│     ├─ SubSplitterTests.cpp
│     └─ ResonatorBankTests.cpp
└─ assets/
   └─ placeholder.txt
```

---

## 11. MVPの機能範囲

### 11.1 MVP必須

| 機能 | 説明 |
|---|---|
| Audio Effect plugin | VST3 FXとして動作 |
| MIDI input | MIDI Chord Mode用 |
| MIDI Chord Mode | MIDIコードからターゲットノートを作る |
| Scale Resonance Mode | Key/Scaleからターゲットノートを作る |
| MIDI入力インジケーター | ルーティング確認用 |
| Active chord display | 現在のMIDIノート表示 |
| Sub Protect | 低域を保護 |
| Transient Bypass | アタックを戻す |
| MIDI連動 Resonator Bank | 中心DSP |
| Amount | 全体の効き |
| Glide | ターゲット周波数の移動 |
| Colour | 倍音/明るさ/共鳴感 |
| Formant UI | PseudoFormantToneを操作 |
| Mix | Dry/Wet |
| Output | 出力 |
| 簡易Spectrum Map UI | 診断表示のみ |
| 状態保存 | DAW保存/復元 |
| 最低限README | 使い方を書く |

### 11.2 v1以降

| 機能 | 優先度 | 説明 |
|---|---|---|
| AU build | v1 | macOS向け |
| Snapshot A/B/C/D | v1 | 設定切り替え |
| CPU quality mode | v1 | Eco / Normal / High |
| Auto gain | v1 | 音量差軽減 |
| Safety limiter | v1 | クリップ防止 |
| Preset 10個 | v1 | すぐ使える状態 |
| Instrument互換版 | v1.5 | MIDIルーティング困難なDAW向け |
| Oversampling toggle | v1.5 | ColourProcessor / drive用 |
| Advanced parameters UI | v1.5 | Resonance / Decay / Drive / Width |
| Formant Gamma | v1.5 | 疑似フォルマント強調 |
| STFT pitch-class mapper | v2 | 本格的なPitchMap風処理へ近づける |
| Custom Map Mode | v2 | GUI上で音を選択 |
| ノートドラッグ編集 | v2 | Pitch lane編集 |
| Convolution IR import | v2 | Cabinet / colour IR |
| Glitch sequencer | v2 | HiTECH / fill用 |
| AI source separation | Experimental | 研究枠 |
| Polyphonic pitch correction | Experimental | 研究枠 |

---

## 12. UI設計

### 12.1 基本思想

UIはNSDuckingに寄せない。

NSColourMapは時間カーブのプラグインではなく、**音高・コード・倍音・スペクトルの変換プラグイン**である。

UIの主役:

- MIDIコード
- スケール
- ピッチレーン
- スペクトル / 倍音の動き
- Sub Protect領域
- Colour処理の強さ
- Transient保持

### 12.2 画面構造

```txt
┌────────────────────────────────────────────────────┐
│ Header / Preset / MIDI Status / CPU / Bypass       │
├────────────────────────────────────────────────────┤
│ Active Chord / Key / Scale / Mode                  │
├────────────────────────────────────────────────────┤
│                                                    │
│              Colour Spectrum Map                   │
│                                                    │
│  input energy / target pitch lanes / sub zone       │
│                                                    │
├────────────────────────────────────────────────────┤
│ Keyboard / Target Notes / MIDI Chord Display        │
├────────────────────────────────────────────────────┤
│ Mode Row: MIDI Chord / Scale Resonance              │
│ Algo Row: Clean / Colour / Hyper / HiTECH / Broken  │
├────────────────────────────────────────────────────┤
│ Macro Controls: Amount / Glide / Colour / Formant   │
│ Utility: SubProtect / Transient / Mix / Output      │
├────────────────────────────────────────────────────┤
│ Snapshots: A B C D                                  │
└────────────────────────────────────────────────────┘
```

### 12.3 UI実装フェーズ

#### UI MVP 1

優先度: `MVP`

- Header
- MIDI LED
- Active chord text
- Mode selector
- Algo selector
- 主要ノブ

#### UI MVP 2

優先度: `MVP〜v1`

- KeyboardView
- Scale note highlight
- MIDI active note highlight
- Sub Protect表示

#### UI MVP 3

優先度: `v1`

- SpectrumMap animation
- Energy blobs
- Target pitch lanes
- Transient flash
- Snapshot UI

### 12.4 SpectrumMapViewの制約

`SpectrumMapView` はMVPでは診断表示である。

MVPで表示してよいもの:

- active MIDI notes
- scale target notes
- resonator target lanes
- simple input energy
- Sub Protect area
- transient flash

MVPで要求してはいけないもの:

- true spectral pitch analysis
- draggable pitch map editor
- STFT resynthesis visualization
- polyphonic source separation display
- PitchMap風の完全編集グリッド

```txt
SpectrumMapView must not block DSP implementation.
If ResonatorBank is not working yet, SpectrumMapView may show placeholder target lanes only.
```

### 12.5 レスポンシブ方針

絶対座標で固定しない。

- 全体を縦積みレイアウトにする
- Headerは最小限
- Spectrum Mapを最も大きく取る
- Keyboardは高さを取りすぎない
- Macro Controlsは小型表示でも操作可能にする
- 幅が狭い場合はノブを2段に折り返す
- 高さが足りない場合はMini KeyboardかSnapshotsを折りたたむ

---

## 13. DSP全体構造

### 13.1 MVP信号フロー

```txt
Audio In
↓
Input Safety / DC Block
↓
Sub Splitter
├─ Low/Sub Path: dry / mono-safe / protected
└─ Mid/High Path
    ↓
    Transient Detector
    ↓
    MIDI Chord / Scale Target Generator
    ↓
    Resonator Bank
    ↓
    Colour Processor
    ↓
    Pseudo Formant Tone Shaper
    ↓
    Wet Gain
↓
Mix Dry/Sub + Wet + Transient Return
↓
Output Gain
↓
Safety Limiter / Clip Guard
↓
Audio Out
```

### 13.2 Audio Threadルール

Audio threadでは以下を禁止。

- メモリ確保
- mutex lock
- ファイルI/O
- ログ出力
- UI操作
- std::vectorのサイズ変更
- 例外を投げる処理

### 13.3 Parameter smoothing

以下のパラメータは必ずスムージングする。

- Amount
- Glide
- Colour
- Formant
- Sub Protect
- Transient
- Mix
- Output
- Resonator frequency
- Resonator gain

---

## 14. MIDI仕様

### 14.1 MIDI入力は前提

このプラグインはMIDI入力を前提とする。  
MIDIが来ない場合でもScale Resonance Modeで動くが、主役はMIDI Chord Mode。

### 14.2 MIDI Chord Mode

MIDI Note Onで、ターゲットノートリストに音を追加する。  
MIDI Note Offで、ターゲットノートリストから音を外す。

ターゲットノートは全オクターブに展開して使う。

```txt
入力MIDI: C3 E3 G3 Bb3
内部ターゲット:
C0 E0 G0 Bb0
C1 E1 G1 Bb1
C2 E2 G2 Bb2
...
C8 E8 G8 Bb8
```

### 14.3 Freeze Last Chord

用意するパラメータ:

```txt
Freeze Last Chord: On/Off
```

初期値:

```txt
On
```

動作:

- MIDIノートが押されている間は、そのコードを使う
- 全Note Off後も、最後のコードを保持する
- 新しいNote Onが来たら更新する

### 14.4 Chord Glide

コードが変わった瞬間に共鳴周波数が飛ぶと耳に痛い。  
Glideでターゲット周波数を滑らかに移動する。

---

## 15. Scale Resonance仕様

### 15.1 Scale Resonance Mode

MIDIコードがないとき用、またはキー固定で使いたいとき用。

```txt
Key: C / C# / D / D# / E / F / F# / G / G# / A / A# / B
Scale:
- Major
- Natural Minor
- Harmonic Minor
- Dorian
- Phrygian
- Lydian
- Mixolydian
- Minor Pentatonic
- Major Pentatonic
- Pentatonic Blues
```

Colour Bass / HiTECH用途の推奨:

```txt
Minor Pentatonic
Pentatonic Blues
Natural Minor
Dorian
Phrygian
```

### 15.2 Scale Shift

```txt
Scale Shift: -12〜+12 semitones
```

### 15.3 誤解防止

```txt
Scale Resonance = スケール音に沿って共鳴/倍音を付ける
Pitch Correction = 入力音のピッチそのものを補正する
```

MVPで作るのは前者。

---

## 16. Sub Protect仕様

### 16.1 目的

Colour Bass系のピッチ/共鳴処理はサブ帯域を濁らせやすい。  
そのため、低域は原音のまま残し、中高域だけ処理する。

### 16.2 動作

```txt
Sub Protect Frequency以下:
- 原音寄り
- Resonator Bankに送らない
- Pseudo Formant Toneに送らない
- Wet処理量を下げる
- Stereo widthを広げない
- 基本はmono-safe
```

初期値:

```txt
120Hz
```

範囲:

```txt
40Hz〜200Hz
```

### 16.3 クロスオーバー実装

優先:

```txt
JUCE dsp::LinkwitzRileyFilter
4th order Linkwitz-Riley style crossover
```

ビルド優先の逃げ道:

```txt
Prefer JUCE dsp::LinkwitzRileyFilter.
If API issues block the build, implement a simple temporary crossover behind the same SubSplitter interface, then replace it later.
```

---

## 17. Transient Bypass仕様

### 17.1 目的

激しい処理モードではアタックが潰れやすい。  
Colour Bass / HiTECHではアタックやノリが大事なので、トランジェントを原音から戻せるようにする。

### 17.2 動作

- 入力信号の急激なエネルギー変化を検出
- トランジェント成分をWet処理から逃がす
- 最後にDry/Transientとして混ぜ戻す

### 17.3 パラメータ

```txt
Transient: 0〜150%
```

- 0%: 戻さない
- 50〜70%: 標準
- 100%以上: アタックを強調

### 17.4 HiTECH用途

```txt
HiTECH default transient: 75%
HiTECH transient release: short
HiTECH wet smear: low
```

---

## 18. Resonator Bank仕様

### 18.1 目的

MIDIコードやスケールに基づいて、入力音にコード感のある共鳴を加える。

### 18.2 構造

```txt
ResonatorBank
├─ ResonatorVoice x N
├─ Target frequency list
├─ Glide smoother
├─ Per-note gain
├─ Q / decay / drive control
├─ Voice normalization
└─ Quality mode
```

### 18.3 Voice数

MVPでは最大32 voices。

```txt
Eco: 12 voices
Normal: 24 voices
High: 32 voices
```

### 18.4 ResonatorVoice

各Voiceは、最低限以下を持つ。

- target frequency
- current frequency
- gain
- Q
- decay
- stereo offset
- drive
- active flag

MVPでは、BandpassまたはState Variable TPT Filterベースで実装してよい。

### 18.5 ResonatorVoiceの2段構成

```txt
1. Narrow Bandpass / SVF Resonator
2. Soft Saturation + High Shelf / Tone shaping
```

各Voiceの出力は、voice countに応じて正規化する。

```txt
voiceGain = baseGain / sqrt(activeVoiceCount)
```

### 18.6 AlgoごとのResonator設定

#### Clean

```txt
Q: low-mid
Decay: short
Drive: off / tiny
Stereo offset: small
Wet: low
```

#### Colour

```txt
Q: medium
Decay: medium
Drive: light
Stereo offset: medium
Wet: medium
```

#### Hyper

```txt
Q: high
Decay: medium-long
Drive: medium
High emphasis: strong
Stereo offset: large
Wet: high
```

#### HiTECH

```txt
Q: medium-high
Decay: short
Drive: medium
Glide: short
Transient keep: high
Stereo offset: controlled
Wet: medium-high
```

#### Broken

```txt
Q: high
Decay: unstable / short-random
Drive: strong
Glide: very short
Detune: slight random
Transient keep: low-mid
Wet: high
```

重要:

```txt
これらは別DSPではない。
同一ResonatorBankの係数・範囲・初期値・マクロ挙動を切り替えるだけで実装する。
```

---

## 19. Colour Processor仕様

### 19.1 目的

Resonator Bankだけだと単なる共鳴になる。  
Colour Bassらしい派手さを出すため、Colour Processorで倍音感、明るさ、密度を足す。

### 19.2 MVP処理候補

MVPでは軽い処理にする。

- gentle saturation
- high harmonic emphasis
- dynamic tone
- stereo width
- filtered feedback風の色付け
- slight noise excitation

### 19.3 パラメータ連動

`Colour` ノブで以下をまとめて制御する。

```txt
Colour up:
- Resonator wet gain up
- high band saturation up
- harmonic density up
- stereo spread slightly up
- PseudoFormantTone amount slightly up
```

---

## 20. Pseudo Formant Tone仕様

### 20.1 名前

UI表示:

```txt
Formant
```

内部クラス名:

```txt
PseudoFormantTone
```

理由:

```txt
MVPでは本物のフォルマントシフターではない。
複数の可動ピークフィルターとtilt EQによる疑似フォルマント処理である。
```

### 20.2 目的

声っぽさ、サイズ感、宇宙人感を作る。

```txt
Formant -:
- 大きい/低い/悪魔っぽい

Formant +:
- 小さい/高い/チップマンクっぽい
```

範囲:

```txt
-24〜+24 semitones
```

内部的には、semitoneを周波数倍率に変換してフィルター中心周波数を動かす。

```txt
ratio = 2 ^ (formantSemitone / 12)
```

### 20.3 MVPで禁止

- phase-vocoder formant shifting
- LPC analysis
- true formant preservation
- spectral envelope reconstruction

---

## 21. Algorithm Modes詳細

### Clean

```txt
Amount: 控えめ
Colour: 控えめ
Transient: 多めに保持
Distortion: なし/弱い
Stereo: 控えめ
```

### Colour

```txt
Amount: 標準
Colour: 標準〜強め
Transient: 標準
Distortion: 軽く
Stereo: 中くらい
```

### Hyper

```txt
Amount: 強め
Colour: 強め
Formant: 動きやすい
Stereo: 広め
High emphasis: 強め
```

### HiTECH

```txt
Amount: 中〜強め
Colour: 強め
Glide: 短め
Decay: 短め
Transient: 高め
Stereo: 中〜広め
Drive: 中
```

HiTECHはBrokenよりも実用寄り。  
破壊ではなく、**速い・細かい・硬い・鮮やか**を狙う。

### Broken

```txt
Amount: 強め
Colour: 強め
Glide: 非常に短め
Transient: 少なめ
Chaos: あり
Drive: 強め
```

---

## 22. パラメータ一覧

| ID | 表示名 | 範囲 | 初期値 | 優先度 | オートメーション |
|---|---|---:|---:|---|---|
| mode | Mode | MIDI Chord / Scale Resonance | MIDI Chord | MVP | Yes |
| algo | Algo | Clean / Colour / Hyper / HiTECH / Broken | Colour | MVP | Yes |
| amount | Amount | 0〜100% | 70% | MVP | Yes |
| glide | Glide | 0〜500ms | 45ms | MVP | Yes |
| colour | Colour | 0〜100% | 55% | MVP | Yes |
| formant | Formant | -24〜+24 st | 0 st | MVP | Yes |
| subProtect | Sub Protect | 40〜200Hz | 120Hz | MVP | Yes |
| transient | Transient | 0〜150% | 60% | MVP | Yes |
| mix | Mix | 0〜100% | 65% | MVP | Yes |
| output | Output | -12〜+12dB | 0dB | MVP | Yes |
| freezeChord | Freeze | Off / On | On | MVP | Yes |
| key | Key | C〜B | C | MVP | Yes |
| scale | Scale | enum | Minor Pentatonic | MVP | Yes |
| scaleShift | Scale Shift | -12〜+12 st | 0 st | MVP | Yes |
| quality | Quality | Eco / Normal / High | Normal | v1 | Optional |

### Advanced parameters候補

| ID | 表示名 | 用途 | 優先度 |
|---|---|---|---|
| resonance | Resonance | Resonator Qの全体調整 | v1 |
| decay | Decay | 共鳴の長さ | v1 |
| drive | Drive | Saturation量 | v1 |
| width | Width | 中高域の広がり | v1 |
| density | Density | voice / harmonic density | v1.5 |
| gate | Gate | HiTECH系の短い切れ味 | v1.5 |
| chaos | Chaos | Broken系の揺れ | v1.5 |
| oversampling | Oversampling | 歪み処理の品質 | v1.5 |

---

## 23. プリセット

優先度: `v1`  
MVPのDSPが動いた後に追加する。

```txt
01 MIDI Colour Bass
02 Soft Chord Wash
03 Resonant Growl
04 Hyper Rainbow
05 HiTECH Laser Fill
06 Broken Formant
07 Noise To Chord
08 Sub Safe Bass
09 Pentatonic Shine
10 Vocal Robot Chord
```

方針:

- 有名アーティスト名をプリセット名に直接使わない
- 用途が分かる名前にする
- MIDI入力が必要なプリセットは名前に `MIDI` を入れる
- Scale Resonance用プリセットはKey/Scale設定込みで保存する
- HiTECH系は短い素材で使う前提の説明を書く

---

## 24. 使い方チュートリアル

### Tutorial 1: MIDIコードでGrowlをColour化

```txt
1. Growl BassトラックにNSColourMapを挿す
2. 別MIDIトラックからコードを送る
3. ModeをMIDI Chordにする
4. AlgoをColourにする
5. Sub Protectを120Hzにする
6. Amountを70%にする
7. Colourを60%にする
8. Mixを50〜70%にする
9. 必要ならサブベースは別トラックで鳴らす
```

### Tutorial 2: Scale Resonanceで雑にColour Bass化

```txt
1. ベース素材にNSColourMapを挿す
2. ModeをScale Resonanceにする
3. Keyを曲のキーに合わせる
4. ScaleをMinor PentatonicまたはPentatonic Bluesにする
5. AlgoをHyperにする
6. Amountを80%にする
7. Sub Protectを120〜150Hzにする
8. Mixを40〜60%にする
```

### Tutorial 3: Noiseをコードテクスチャ化

```txt
1. Noise / Metallic textureにNSColourMapを挿す
2. ModeをMIDI Chordにする
3. MIDIでコードを送る
4. AlgoをBrokenまたはHyperにする
5. Colourを80%以上にする
6. Transientを低めにする
7. Mixを100%寄りにする
```

### Tutorial 4: Vocal Chopを変形する

```txt
1. Vocal ChopにNSColourMapを挿す
2. ModeをMIDI Chordにする
3. AlgoをColourまたはBrokenにする
4. Formantをオートメーションする
5. Glideを短めにする
6. Mixを60〜100%にする
```

### Tutorial 5: HiTECH Laser Fill

```txt
1. Short Noise / FM Zap / Metallic One ShotにNSColourMapを挿す
2. ModeをMIDI ChordまたはScale Resonanceにする
3. AlgoをHiTECHにする
4. Glideを5〜25msにする
5. Colourを70〜90%にする
6. Transientを70〜100%にする
7. Formantを高速オートメーションする
8. Mixを60〜90%にする
```

---

## 25. フェーズ分け

### Phase 0: Repository Setup — MVP

目的:

- 空プロジェクトをビルド可能にする

完了条件:

- VST3がビルドできる
- 空のUIが表示される
- MIDI入力設定を有効にしている

### Phase 1: Parameter + State Foundation — MVP

完了条件:

- DAWでパラメータが見える
- 保存/復元できる
- UIなしでもパラメータが動く

### Phase 2: MIDI Foundation — MVP

完了条件:

- MIDIノートを受けるとUIまたはdebug表示に出る
- Note Offで状態が更新される
- Freeze Last Chordが動く

### Phase 3: Scale Resonance Foundation — MVP

完了条件:

- C Minor Pentatonicなどのスケール音が生成できる
- Scale Shiftでターゲットが移動する
- MIDI Chord ModeとScale Resonance Modeを切り替えられる

ここまでが安定してビルドできるまで、Phase 4以降に進まない。

### Phase 4: Sub Split + Transient Bypass — MVP

完了条件:

- Sub Protect以下がWet処理されない
- Transientパラメータでアタックが戻る
- バイパス時と通常時で音量差が大きく破綻しない

### Phase 5: Resonator Bank MVP — MVP

完了条件:

- MIDIコードを変えると共鳴音が変わる
- Scale Resonanceでもターゲットノートに沿った共鳴が出る
- Sub Protectが効いている
- CPUが極端に重くない

### Phase 6: UI MVP 1 — MVP

完了条件:

- UIから主要パラメータを操作できる
- MIDI状態が分かる
- 最低限プラグインとして触れる

### Phase 7: Colour Processor + Pseudo Formant + Algo Tuning — MVP〜v1

完了条件:

- Algoごとの違いが分かる
- Formantで音色が動く
- Colourでコード感と明るさが増える
- HiTECHで短く硬いフィルが作れる

### Phase 8: UI MVP 2/3 — v1

完了条件:

- MIDIノートが見える
- Scale音が見える
- Sub Protect領域が見える
- Spectrum Mapが最低限動く

### Phase 9: Presets + Polish — v1

完了条件:

- プリセットを選んで音作りが始められる
- READMEに使い方がある
- DAWで保存/復元できる
- VST3として配布できる

### Phase 10: Advanced / v2

内容:

- STFT pitch-class mapper
- Custom Map Mode
- ノートドラッグ編集
- Convolution IR import
- Glitch sequencer
- true spectral visualization
- Instrument互換版
- standalone app

---

## 26. ClaudeCode用タスク分割

### Task 1

```txt
Create a minimal JUCE/CMake plugin project named NSColourMap.
It must build a VST3 audio effect with MIDI input enabled.
Do not implement DSP yet.
Create README.md, AGENTS.md, docs/NSColourMap_Spec.md, docs/Research_Notes.md, docs/ClaudeCode_Tasks.md, and basic Source files.
```

### Task 2

```txt
Implement AudioProcessorValueTreeState parameters for Mode, Algo, Amount, Glide, Colour, Formant, SubProtect, Transient, Mix, Output, FreezeChord, Key, Scale, ScaleShift, and Quality.
Do not implement UI polish yet.
```

### Task 3

```txt
Implement MidiChordState with Note On/Off handling, active note tracking, Freeze Last Chord, and octave-expanded target pitch generation.
Add small unit tests or debug assertions.
```

### Task 4

```txt
Implement ScaleNoteSet and Scale Resonance target generation.
Support Major, Natural Minor, Harmonic Minor, Dorian, Phrygian, Lydian, Mixolydian, Minor Pentatonic, Major Pentatonic, and Pentatonic Blues.
Do not implement pitch correction.
```

### Task 5

```txt
Implement SubSplitter and TransientDetector.
Sub Protect should keep low frequencies mostly dry and mono-safe.
Use a Linkwitz-Riley style crossover if possible.
If JUCE API issues block the build, keep the SubSplitter interface and use a temporary simple crossover.
Transient parameter should mix transient detail back after wet processing.
```

### Task 6

```txt
Implement a simple ResonatorBank controlled by MIDI Chord Mode or Scale Resonance Mode.
Use simple filter-based resonators first.
Avoid FFT, STFT resynthesis, or complex source separation.
Normalize voice output to avoid gain jumps.
```

### Task 7

```txt
Implement UI MVP 1:
Header, MIDI indicator, active chord text, Mode/Algo buttons, and macro controls.
Use responsive layout.
Do not use absolute fixed SVG coordinates.
```

### Task 8

```txt
Implement ColourProcessor and PseudoFormantTone.
Use light saturation, harmonic emphasis, stereo width, tilt EQ, and movable peak filters.
Do not implement a true formant shifter.
```

### Task 9

```txt
Tune the five algorithm modes: Clean, Colour, Hyper, HiTECH, Broken.
Make them clearly different using existing parameters and simple DSP.
Do not create separate DSP engines for each algorithm.
```

### Task 10

```txt
Implement UI MVP 2/3:
KeyboardView, target note highlights, Sub Protect area, SpectrumMap placeholder, and Snapshot placeholders.
Do not block DSP progress for advanced animation.
SpectrumMapView is diagnostic only in the MVP.
```

---

## 27. 音作りの初期値

### MIDI Colour Bass

```txt
Mode: MIDI Chord
Algo: Colour
Amount: 70%
Glide: 45ms
Colour: 60%
Formant: 0st
Sub Protect: 120Hz
Transient: 60%
Mix: 65%
```

### Hyper Rainbow

```txt
Mode: MIDI Chord
Algo: Hyper
Amount: 85%
Glide: 20ms
Colour: 85%
Formant: +7st
Sub Protect: 130Hz
Transient: 45%
Mix: 75%
```

### Soft Scale Resonance

```txt
Mode: Scale Resonance
Algo: Clean
Key: song key
Scale: Minor Pentatonic
Amount: 45%
Glide: 90ms
Colour: 30%
Sub Protect: 120Hz
Mix: 40%
```

### HiTECH Laser Fill

```txt
Mode: MIDI Chord or Scale Resonance
Algo: HiTECH
Amount: 80%
Glide: 12ms
Colour: 85%
Formant: +12st or automated
Sub Protect: 140Hz
Transient: 85%
Mix: 75%
```

### Broken Formant

```txt
Mode: MIDI Chord
Algo: Broken
Amount: 90%
Glide: 8ms
Colour: 90%
Formant: automated
Sub Protect: 150Hz
Transient: 20%
Mix: 90%
```

---

## 28. READMEに入れるべき文章

### 28.1 冒頭説明

```md
NSColourMap is a free JUCE-based VST3/AU effect for Colour Bass and HiTECH sound design.

It turns basses, noise, vocal chops, and metallic textures into MIDI/scale-aware harmonic colour textures using a resonator bank, sub protection, transient bypass, and simplified formant/tone processing.

It is not a true PitchMap clone or a polyphonic pitch correction engine.
```

### 28.2 MIDIが来ないときの説明

```md
If the plugin does not react to your chords, check the MIDI LED.
If MIDI routing is difficult in your DAW, use Scale Resonance Mode first.
```

### 28.3 Safety説明

```md
For bass-heavy material, keep Sub Protect around 100-150 Hz.
For full basses, use Mix around 40-70% and keep a separate clean sub if needed.
```

---

## 29. docs/Research_Notes.mdに入れる内容

```md
# Research Notes

## Colour Bass Workflow Summary

Common approaches:
- Vocoder with MIDI chords
- Resonator / Spectral Resonator tuned to chord notes
- PitchMap-style pitch remapping
- Convolution / cabinet / reverb for tone
- OTT / saturation / multiband after harmonic processing

## HiTECH / Glitch Workflow Summary

Common needs:
- short transients
- laser/fill sounds
- FM/noise/metallic one-shots
- fast automation
- stable low end
- controlled stereo in highs only

## What NSColourMap takes from these workflows

- MIDI chord control from vocoder and PitchMap-style workflows
- Resonator bank as the practical MVP core
- Sub Protect because bass processing often muddies low end
- Transient Bypass because Colour Bass and HiTECH need attack
- Pseudo Formant Tone for vocal/robot/alien timbre
- HiTECH as a tuning mode for fast, hard, short sounds

## What NSColourMap does not do in MVP

- No true polyphonic pitch correction
- No source separation
- No STFT resynthesis
- No PitchMap UI clone
- No separate HiTECH DSP engine
```

---

## 30. テスト方針

### 30.1 Unit Tests

最低限、以下はテストする。

- ScaleNoteSet
- MidiChordState
- TargetNoteGenerator
- Freeze Last Chord
- parameter ranges
- ResonatorBank voice allocation
- SubSplitter recombination gain

### 30.2 Manual DAW Tests

最低限、以下で確認する。

- Ableton Live
- FL Studio
- Logic Pro
- REAPER

確認項目:

- MIDIをAudio Effectへ送れるか
- VST3が読み込めるか
- AUが読み込めるか
- パラメータ保存されるか
- 音切れ/クラッシュがないか
- CPU負荷が重すぎないか
- MIDIが来ないときにScale Resonanceで使えるか

### 30.3 Audio Quality Tests

最低限、以下を確認する。

- Sub Protect 120Hzで低域が痩せない
- Mix 0%でDryに近い音になる
- Mix 100%でWetのみになる
- Outputを上げても急に破綻しない
- MIDIコード変更時にクリックが出にくい
- Transientを上げるとアタックが戻る
- HiTECHで短い素材が潰れすぎない

---

## 31. 完了条件

MVP完了と呼べる条件:

- VST3としてDAWに読み込める
- MIDI入力を受け取れる
- MIDI Chord Modeでコードに応じて音が変わる
- Scale Resonance Modeでキー/スケールに応じた音になる
- Sub Protectで低域が濁りにくい
- Transientでアタックを戻せる
- ColourノブでColour Bassっぽい共鳴感が増える
- Formantで疑似的に音色が変わる
- HiTECH Algoで短く硬いフィル/レーザー感が作れる
- MixでDry/Wet調整できる
- UIでMIDI状態とターゲット音が最低限見える
- SpectrumMapは診断表示として動く
- プロジェクト保存/復元でパラメータが戻る
- クラッシュしない
- READMEに使い方とMIDIルーティングが書いてある

---

## 32. 一文まとめ

**NSColourMapは、MIDIコード入力とスケール指定を中心に、Resonator / Vocoder / PitchMap系のColour Bass制作ワークフローを、軽量で実装可能なハーモニックカラー変換VSTとしてまとめるプロジェクトである。**

MVPの正体は、**PitchMapクローンではなく、MIDI / Scale-aware Resonator Bank + Colour Processor + Sub Protect + Transient Bypass + Pseudo Formant Tone** である。

機能は削除しない。  
ただし、MVPで音が出る核を先に完成させ、STFT、Custom Map、Advanced UI、Experimental DSPは後の段階へ送る。

これにより、Colour Bassのコード感、HiTECHの短く硬いフィル、ノイズ/ボーカル/ベースの音楽的な変形を、現実的な実装難度で実現する。
