#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;

namespace MatchstickPuzzle {

public enum class Difficulty
{
    Easy,
    Medium,
    Hard
};

public enum class GameMode
{
    Campaign,
    Timed,
    Free
};

public ref class Level
{
public:
    String^ Title;
    String^ Start;
    String^ Target;
    String^ Hint;
    Difficulty Rank;
    int Moves;

    Level(String^ title, String^ start, String^ target, int moves, Difficulty rank, String^ hint)
    {
        Title = title;
        Start = start;
        Target = target;
        Moves = moves;
        Rank = rank;
        Hint = hint;
    }
};

public ref class SegmentSlot
{
public:
    int Position;
    int Segment;
    RectangleF Bounds;
    PointF A;
    PointF B;
    bool Horizontal;
};

public ref class BoardControl : public Control
{
private:
    List<SegmentSlot^>^ slots;
    array<bool>^ current;
    array<bool>^ target;
    EventHandler^ boardChangedHandlers;
    int selected;

public:
    event EventHandler^ BoardChanged
    {
        void add(EventHandler^ handler)
        {
            boardChangedHandlers =
                safe_cast<EventHandler^>(Delegate::Combine(boardChangedHandlers, handler));
        }

        void remove(EventHandler^ handler)
        {
            boardChangedHandlers =
                safe_cast<EventHandler^>(Delegate::Remove(boardChangedHandlers, handler));
        }
    }

    BoardControl()
    {
        slots = gcnew List<SegmentSlot^>();
        current = gcnew array<bool>(0);
        target = gcnew array<bool>(0);
        selected = -1;
        DoubleBuffered = true;
        BackColor = Color::FromArgb(248, 249, 250);
        Cursor = Cursors::Hand;
        SetStyle(ControlStyles::ResizeRedraw, true);
    }

    property int SelectedIndex
    {
        int get() { return selected; }
    }

    property int ActiveCount
    {
        int get()
        {
            int count = 0;
            for each (bool value in current)
                if (value) count++;
            return count;
        }
    }

    void LoadLevel(Level^ level)
    {
        BuildSlots();
        current = StateFromExpression(level->Start);
        target = StateFromExpression(level->Target);
        selected = -1;
        Invalidate();
    }

    void ResetState(Level^ level)
    {
        current = StateFromExpression(level->Start);
        selected = -1;
        Invalidate();
    }

    bool IsSolved()
    {
        if (current->Length != target->Length) return false;
        for (int i = 0; i < current->Length; ++i)
            if (current[i] != target[i]) return false;
        return true;
    }

    String^ ReadExpression()
    {
        if (current->Length == 0) return "";
        array<wchar_t>^ chars = gcnew array<wchar_t>(5);
        chars[0] = ReadDigit(0);
        chars[1] = ReadOperator(1);
        chars[2] = ReadDigit(2);
        chars[3] = ReadEquals(3);
        chars[4] = ReadDigit(4);
        return gcnew String(chars);
    }

protected:
    virtual void OnResize(EventArgs^ e) override
    {
        Control::OnResize(e);
        BuildSlots();
        Invalidate();
    }

    virtual void OnPaint(PaintEventArgs^ e) override
    {
        Control::OnPaint(e);
        Graphics^ g = e->Graphics;
        g->SmoothingMode = SmoothingMode::AntiAlias;
        g->Clear(BackColor);

        if (slots->Count == 0) BuildSlots();

        DrawBoardBackplate(g);

        for (int i = 0; i < slots->Count; ++i)
        {
            bool isActive = i < current->Length && current[i];
            bool isSelected = i == selected;
            Color color = isActive ? Color::FromArgb(219, 80, 48) : Color::FromArgb(210, 216, 223);
            if (isSelected) color = Color::FromArgb(33, 112, 181);

            Pen^ pen = gcnew Pen(color, isActive ? 12.0f : 8.0f);
            pen->StartCap = LineCap::Round;
            pen->EndCap = LineCap::Round;
            g->DrawLine(pen, slots[i]->A, slots[i]->B);

            if (!isActive)
            {
                Pen^ outline = gcnew Pen(Color::FromArgb(176, 186, 197), 1.0f);
                outline->DashStyle = DashStyle::Dot;
                g->DrawLine(outline, slots[i]->A, slots[i]->B);
            }
        }

        DrawReadout(g);
    }

    virtual void OnMouseClick(MouseEventArgs^ e) override
    {
        Control::OnMouseClick(e);
        int hit = HitTest(e->Location);
        if (hit < 0 || hit >= current->Length) return;

        if (current[hit])
        {
            selected = hit;
            Invalidate();
            return;
        }

        if (selected >= 0 && current[selected] && !current[hit])
        {
            current[selected] = false;
            current[hit] = true;
            selected = -1;
            Invalidate();
            RaiseChanged();
        }
    }

private:
    void RaiseChanged()
    {
        if (boardChangedHandlers != nullptr)
            boardChangedHandlers(this, EventArgs::Empty);
    }

    void DrawBoardBackplate(Graphics^ g)
    {
        Rectangle rect(10, 10, Math::Max(20, Width - 20), Math::Max(20, Height - 20));
        GraphicsPath^ path = RoundedRect(rect, 8);
        SolidBrush^ brush = gcnew SolidBrush(Color::FromArgb(255, 255, 255));
        Pen^ pen = gcnew Pen(Color::FromArgb(226, 232, 240), 1.0f);
        g->FillPath(brush, path);
        g->DrawPath(pen, path);
    }

    void DrawReadout(Graphics^ g)
    {
        String^ text = ReadExpression();
        if (String::IsNullOrWhiteSpace(text)) return;

        System::Drawing::Font^ font = gcnew System::Drawing::Font("Segoe UI", 14.0f, FontStyle::Bold);
        SolidBrush^ brush = gcnew SolidBrush(Color::FromArgb(72, 84, 96));
        StringFormat^ format = gcnew StringFormat();
        format->Alignment = StringAlignment::Center;
        RectangleF rect(0, (float)Height - 38.0f, (float)Width, 28.0f);
        g->DrawString(L"Сейчас: " + text, font, brush, rect, format);
    }

    GraphicsPath^ RoundedRect(Rectangle rect, int radius)
    {
        GraphicsPath^ path = gcnew GraphicsPath();
        int d = radius * 2;
        path->AddArc(rect.X, rect.Y, d, d, 180, 90);
        path->AddArc(rect.Right - d, rect.Y, d, d, 270, 90);
        path->AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
        path->AddArc(rect.X, rect.Bottom - d, d, d, 90, 90);
        path->CloseFigure();
        return path;
    }

    int HitTest(Point location)
    {
        for (int i = 0; i < slots->Count; ++i)
            if (slots[i]->Bounds.Contains((float)location.X, (float)location.Y))
                return i;
        return -1;
    }

    void BuildSlots()
    {
        slots->Clear();
        float totalWidth = 520.0f;
        float startX = (float)Math::Max(22.0, ((double)Width - totalWidth) / 2.0);
        float y = (float)Math::Max(36.0, ((double)Height - 190.0) / 2.0);

        AddDigitSlots(0, startX, y);
        AddOperatorSlots(1, startX + 112.0f, y + 34.0f, false);
        AddDigitSlots(2, startX + 196.0f, y);
        AddOperatorSlots(3, startX + 308.0f, y + 34.0f, true);
        AddDigitSlots(4, startX + 392.0f, y);
    }

    void AddSlot(int position, int segment, PointF a, PointF b, bool horizontal)
    {
        SegmentSlot^ slot = gcnew SegmentSlot();
        slot->Position = position;
        slot->Segment = segment;
        slot->A = a;
        slot->B = b;
        slot->Horizontal = horizontal;

        float left = Math::Min(a.X, b.X) - 14.0f;
        float top = Math::Min(a.Y, b.Y) - 14.0f;
        float right = Math::Max(a.X, b.X) + 14.0f;
        float bottom = Math::Max(a.Y, b.Y) + 14.0f;
        slot->Bounds = RectangleF(left, top, right - left, bottom - top);
        slots->Add(slot);
    }

    void AddDigitSlots(int position, float x, float y)
    {
        float l = x + 18.0f;
        float r = x + 76.0f;
        float t = y + 8.0f;
        float m = y + 74.0f;
        float b = y + 140.0f;
        AddSlot(position, 0, PointF(l, t), PointF(r, t), true);
        AddSlot(position, 1, PointF(l - 8.0f, t + 10.0f), PointF(l - 8.0f, m - 10.0f), false);
        AddSlot(position, 2, PointF(r + 8.0f, t + 10.0f), PointF(r + 8.0f, m - 10.0f), false);
        AddSlot(position, 3, PointF(l, m), PointF(r, m), true);
        AddSlot(position, 4, PointF(l - 8.0f, m + 10.0f), PointF(l - 8.0f, b - 10.0f), false);
        AddSlot(position, 5, PointF(r + 8.0f, m + 10.0f), PointF(r + 8.0f, b - 10.0f), false);
        AddSlot(position, 6, PointF(l, b), PointF(r, b), true);
    }

    void AddOperatorSlots(int position, float x, float y, bool equalsSign)
    {
        if (equalsSign)
        {
            AddSlot(position, 0, PointF(x + 8.0f, y + 40.0f), PointF(x + 64.0f, y + 40.0f), true);
            AddSlot(position, 1, PointF(x + 8.0f, y + 76.0f), PointF(x + 64.0f, y + 76.0f), true);
        }
        else
        {
            AddSlot(position, 0, PointF(x + 8.0f, y + 58.0f), PointF(x + 64.0f, y + 58.0f), true);
            AddSlot(position, 1, PointF(x + 36.0f, y + 30.0f), PointF(x + 36.0f, y + 86.0f), false);
        }
    }

    array<bool>^ StateFromExpression(String^ expression)
    {
        array<bool>^ state = gcnew array<bool>(slots->Count);
        for (int i = 0; i < expression->Length && i < 5; ++i)
        {
            array<bool>^ mask = MaskFor(expression[i], i);
            int maskIndex = 0;
            for (int s = 0; s < slots->Count; ++s)
            {
                if (slots[s]->Position == i)
                {
                    state[s] = maskIndex < mask->Length && mask[maskIndex];
                    ++maskIndex;
                }
            }
        }
        return state;
    }

    array<bool>^ MaskFor(wchar_t c, int position)
    {
        if (position == 1)
        {
            array<bool>^ op = gcnew array<bool>(2);
            op[0] = c == L'+' || c == L'-';
            op[1] = c == L'+';
            return op;
        }
        if (position == 3)
        {
            array<bool>^ eq = gcnew array<bool>(2);
            eq[0] = c == L'=';
            eq[1] = c == L'=';
            return eq;
        }

        array<bool>^ d = gcnew array<bool>(7);
        switch (c)
        {
        case L'0': return DigitMask(true, true, true, false, true, true, true);
        case L'1': return DigitMask(false, false, true, false, false, true, false);
        case L'2': return DigitMask(true, false, true, true, true, false, true);
        case L'3': return DigitMask(true, false, true, true, false, true, true);
        case L'4': return DigitMask(false, true, true, true, false, true, false);
        case L'5': return DigitMask(true, true, false, true, false, true, true);
        case L'6': return DigitMask(true, true, false, true, true, true, true);
        case L'7': return DigitMask(true, false, true, false, false, true, false);
        case L'8': return DigitMask(true, true, true, true, true, true, true);
        case L'9': return DigitMask(true, true, true, true, false, true, true);
        default: return d;
        }
    }

    array<bool>^ DigitMask(bool a, bool b, bool c, bool d, bool e, bool f, bool g)
    {
        array<bool>^ mask = gcnew array<bool>(7);
        mask[0] = a; mask[1] = b; mask[2] = c; mask[3] = d;
        mask[4] = e; mask[5] = f; mask[6] = g;
        return mask;
    }

    wchar_t ReadDigit(int position)
    {
        for (wchar_t c = L'0'; c <= L'9'; ++c)
        {
            array<bool>^ mask = MaskFor(c, position);
            bool same = true;
            int maskIndex = 0;
            for (int i = 0; i < slots->Count; ++i)
            {
                if (slots[i]->Position != position) continue;
                if (current[i] != mask[maskIndex]) same = false;
                ++maskIndex;
            }
            if (same) return c;
        }
        return L'?';
    }

    wchar_t ReadOperator(int position)
    {
        bool horizontal = false;
        bool vertical = false;
        for (int i = 0; i < slots->Count; ++i)
        {
            if (slots[i]->Position != position) continue;
            if (slots[i]->Segment == 0) horizontal = current[i];
            if (slots[i]->Segment == 1) vertical = current[i];
        }
        if (horizontal && vertical) return L'+';
        if (horizontal && !vertical) return L'-';
        return L'?';
    }

    wchar_t ReadEquals(int position)
    {
        bool top = false;
        bool bottom = false;
        for (int i = 0; i < slots->Count; ++i)
        {
            if (slots[i]->Position != position) continue;
            if (slots[i]->Segment == 0) top = current[i];
            if (slots[i]->Segment == 1) bottom = current[i];
        }
        return top && bottom ? L'=' : L'?';
    }
};

public ref class MainForm : public Form
{
private:
    BoardControl^ board;
    Label^ titleLabel;
    Label^ infoLabel;
    Label^ statusLabel;
    Label^ timerLabel;
    ComboBox^ difficultyBox;
    ComboBox^ modeBox;
    Button^ resetButton;
    Button^ hintButton;
    Button^ nextButton;
    Timer^ gameTimer;
    List<Level^>^ levels;
    List<Level^>^ activeLevels;
    Level^ currentLevel;
    GameMode mode;
    Difficulty difficulty;
    int levelIndex;
    int movesUsed;
    int secondsLeft;
    int score;

public:
    MainForm()
    {
        Text = L"Головоломка со спичками";
        MinimumSize = System::Drawing::Size(820, 560);
        Size = System::Drawing::Size(980, 650);
        StartPosition = FormStartPosition::CenterScreen;
        Font = gcnew System::Drawing::Font("Segoe UI", 10.0f);
        BackColor = Color::FromArgb(244, 246, 248);

        mode = GameMode::Campaign;
        difficulty = Difficulty::Easy;
        levelIndex = 0;
        movesUsed = 0;
        secondsLeft = 180;
        score = 0;

        BuildLevels();
        BuildInterface();
        StartGame();
    }

private:
    void BuildInterface()
    {
        TableLayoutPanel^ root = gcnew TableLayoutPanel();
        root->Dock = DockStyle::Fill;
        root->ColumnCount = 1;
        root->RowCount = 4;
        root->Padding = System::Windows::Forms::Padding(18);
        root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 76));
        root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 62));
        root->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
        root->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 54));
        Controls->Add(root);

        Panel^ header = gcnew Panel();
        header->Dock = DockStyle::Fill;
        root->Controls->Add(header, 0, 0);

        titleLabel = gcnew Label();
        titleLabel->Text = L"Головоломка со спичками";
        titleLabel->AutoSize = true;
        titleLabel->Font = gcnew System::Drawing::Font("Segoe UI Semibold", 22.0f, FontStyle::Bold);
        titleLabel->ForeColor = Color::FromArgb(31, 41, 55);
        titleLabel->Location = Point(0, 2);
        header->Controls->Add(titleLabel);

        timerLabel = gcnew Label();
        timerLabel->Anchor = AnchorStyles::Top | AnchorStyles::Right;
        timerLabel->AutoSize = false;
        timerLabel->TextAlign = ContentAlignment::MiddleRight;
        timerLabel->Font = gcnew System::Drawing::Font("Segoe UI Semibold", 14.0f, FontStyle::Bold);
        timerLabel->ForeColor = Color::FromArgb(33, 112, 181);
        timerLabel->Bounds = Rectangle(Width - 260, 12, 200, 36);
        header->Controls->Add(timerLabel);
        header->Resize += gcnew EventHandler(this, &MainForm::HeaderResized);

        FlowLayoutPanel^ tools = gcnew FlowLayoutPanel();
        tools->Dock = DockStyle::Fill;
        tools->WrapContents = false;
        tools->FlowDirection = FlowDirection::LeftToRight;
        tools->Padding = System::Windows::Forms::Padding(0, 8, 0, 0);
        root->Controls->Add(tools, 0, 1);

        modeBox = gcnew ComboBox();
        modeBox->DropDownStyle = ComboBoxStyle::DropDownList;
        modeBox->Width = 150;
        modeBox->Items->AddRange(gcnew array<Object^>{
            gcnew String(L"Кампания"), gcnew String(L"На время"), gcnew String(L"Свободно")});
        modeBox->SelectedIndex = 0;
        modeBox->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::ModeChanged);
        tools->Controls->Add(modeBox);

        difficultyBox = gcnew ComboBox();
        difficultyBox->DropDownStyle = ComboBoxStyle::DropDownList;
        difficultyBox->Width = 150;
        difficultyBox->Items->AddRange(gcnew array<Object^>{
            gcnew String(L"Легко"), gcnew String(L"Средне"), gcnew String(L"Сложно")});
        difficultyBox->SelectedIndex = 0;
        difficultyBox->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::DifficultyChanged);
        tools->Controls->Add(difficultyBox);

        resetButton = CreateButton(L"Сбросить");
        resetButton->Click += gcnew EventHandler(this, &MainForm::ResetClicked);
        tools->Controls->Add(resetButton);

        hintButton = CreateButton(L"Подсказка");
        hintButton->Click += gcnew EventHandler(this, &MainForm::HintClicked);
        tools->Controls->Add(hintButton);

        nextButton = CreateButton(L"Следующий");
        nextButton->Click += gcnew EventHandler(this, &MainForm::NextClicked);
        tools->Controls->Add(nextButton);

        board = gcnew BoardControl();
        board->Dock = DockStyle::Fill;
        board->BoardChanged += gcnew EventHandler(this, &MainForm::BoardChanged);
        root->Controls->Add(board, 0, 2);

        Panel^ footer = gcnew Panel();
        footer->Dock = DockStyle::Fill;
        root->Controls->Add(footer, 0, 3);

        infoLabel = gcnew Label();
        infoLabel->AutoSize = false;
        infoLabel->Dock = DockStyle::Top;
        infoLabel->Height = 26;
        infoLabel->ForeColor = Color::FromArgb(55, 65, 81);
        footer->Controls->Add(infoLabel);

        statusLabel = gcnew Label();
        statusLabel->AutoSize = false;
        statusLabel->Dock = DockStyle::Bottom;
        statusLabel->Height = 24;
        statusLabel->ForeColor = Color::FromArgb(107, 114, 128);
        footer->Controls->Add(statusLabel);

        gameTimer = gcnew Timer();
        gameTimer->Interval = 1000;
        gameTimer->Tick += gcnew EventHandler(this, &MainForm::TimerTick);
    }

    Button^ CreateButton(String^ text)
    {
        Button^ button = gcnew Button();
        button->Text = text;
        button->Width = 116;
        button->Height = 34;
        button->FlatStyle = FlatStyle::Flat;
        button->FlatAppearance->BorderColor = Color::FromArgb(203, 213, 225);
        button->BackColor = Color::White;
        button->ForeColor = Color::FromArgb(31, 41, 55);
        return button;
    }

    void BuildLevels()
    {
        levels = gcnew List<Level^>();
        levels->Add(gcnew Level(L"Разминка", "6+4=4", "8-4=4", 1, Difficulty::Easy,
            L"Снимите вертикальную спичку у плюса и добавьте ее к первой цифре."));
        levels->Add(gcnew Level(L"Смена знака", "1+1=6", "7-1=6", 1, Difficulty::Easy,
            L"Вертикальная спичка плюса может стать частью первой цифры."));
        levels->Add(gcnew Level(L"Баланс", "2+2=5", "3+2=5", 1, Difficulty::Easy,
            L"Достройте первую цифру, забрав лишнюю спичку из результата."));
        levels->Add(gcnew Level(L"Двойной фокус", "1+1=9", "7-1=6", 2, Difficulty::Medium,
            L"Сначала подумайте о знаке, затем о крайней правой цифре."));
        levels->Add(gcnew Level(L"Точная сумма", "1+3=8", "7+2=9", 2, Difficulty::Medium,
            L"Два переноса меняют каждую сторону равенства."));
        levels->Add(gcnew Level(L"Переход через восемь", "1+4=6", "7+1=8", 2, Difficulty::Medium,
            L"Нужно получить семерку слева и восьмерку справа."));
        levels->Add(gcnew Level(L"Сложная перестройка", "1+1=8", "7-4=3", 3, Difficulty::Hard,
            L"Три переноса меняют знак, среднюю цифру и результат."));
        levels->Add(gcnew Level(L"Финальная логика", "1+2=7", "7-6=1", 3, Difficulty::Hard,
            L"Ищите решение через минус: левая часть должна стать 7-6."));
    }

    void StartGame()
    {
        activeLevels = gcnew List<Level^>();
        for each (Level^ level in levels)
            if (level->Rank == difficulty || mode == GameMode::Free)
                activeLevels->Add(level);

        if (activeLevels->Count == 0)
            activeLevels->AddRange(levels);

        levelIndex = 0;
        score = 0;
        secondsLeft = mode == GameMode::Timed ? 180 : 0;
        gameTimer->Enabled = mode == GameMode::Timed;
        LoadCurrentLevel();
    }

    void LoadCurrentLevel()
    {
        currentLevel = activeLevels[levelIndex % activeLevels->Count];
        movesUsed = 0;
        board->Enabled = true;
        board->LoadLevel(currentLevel);
        UpdateLabels(L"Выберите спичку, затем пустое место для переноса.");
    }

    void UpdateLabels(String^ status)
    {
        String^ rank = difficultyBox->SelectedItem == nullptr ? "" : difficultyBox->SelectedItem->ToString();
        String^ modeName = modeBox->SelectedItem == nullptr ? "" : modeBox->SelectedItem->ToString();
        int limit = MoveLimit();
        infoLabel->Text = String::Format(L"{0}: {1}   |   Цель: {2}   |   Ходы: {3}/{4}   |   Режим: {5}, {6}",
            currentLevel->Title, currentLevel->Start, currentLevel->Target, movesUsed, limit, modeName, rank);
        statusLabel->Text = status;
        timerLabel->Text = mode == GameMode::Timed
            ? String::Format(L"Время: {0}:{1:00}   Очки: {2}", secondsLeft / 60, secondsLeft % 60, score)
            : String::Format(L"Очки: {0}", score);
    }

    int MoveLimit()
    {
        return mode == GameMode::Free ? currentLevel->Moves + 3 : currentLevel->Moves;
    }

    void BoardChanged(Object^, EventArgs^)
    {
        ++movesUsed;

        if (board->IsSolved())
        {
            score += Math::Max(10, 60 - movesUsed * 8);
            UpdateLabels(L"Уровень решен. Отлично.");
            MessageBox::Show(L"Решено! Переходим дальше.", L"Победа", MessageBoxButtons::OK, MessageBoxIcon::Information);
            NextLevel();
            return;
        }

        if (movesUsed >= MoveLimit())
        {
            UpdateLabels(L"Лимит ходов исчерпан. Можно сбросить уровень или открыть подсказку.");
            board->Enabled = false;
            return;
        }

        UpdateLabels(L"Ход принят. Продолжайте искать верное равенство.");
    }

    void NextLevel()
    {
        levelIndex = (levelIndex + 1) % activeLevels->Count;
        LoadCurrentLevel();
    }

    void HeaderResized(Object^ sender, EventArgs^)
    {
        Control^ header = safe_cast<Control^>(sender);
        timerLabel->Bounds = Rectangle(Math::Max(0, header->Width - 310), 12, 300, 36);
    }

    void ModeChanged(Object^, EventArgs^)
    {
        mode = static_cast<GameMode>(modeBox->SelectedIndex);
        StartGame();
    }

    void DifficultyChanged(Object^, EventArgs^)
    {
        difficulty = static_cast<Difficulty>(difficultyBox->SelectedIndex);
        StartGame();
    }

    void ResetClicked(Object^, EventArgs^)
    {
        movesUsed = 0;
        board->Enabled = true;
        board->ResetState(currentLevel);
        UpdateLabels(L"Уровень сброшен.");
    }

    void HintClicked(Object^, EventArgs^)
    {
        MessageBox::Show(currentLevel->Hint, L"Подсказка", MessageBoxButtons::OK, MessageBoxIcon::Information);
    }

    void NextClicked(Object^, EventArgs^)
    {
        NextLevel();
    }

    void TimerTick(Object^, EventArgs^)
    {
        if (secondsLeft > 0)
        {
            --secondsLeft;
            UpdateLabels(statusLabel->Text);
            return;
        }

        gameTimer->Stop();
        MessageBox::Show(String::Format(L"Время вышло. Ваш счет: {0}", score), L"Игра окончена",
            MessageBoxButtons::OK, MessageBoxIcon::Information);
        StartGame();
    }
};
}
