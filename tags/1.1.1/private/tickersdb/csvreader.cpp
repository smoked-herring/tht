#include "csvreader.h"
#include <QtCore/QTextDecoder>


CsvParser::CsvParser()
    :src(QString::null)
    , pos(0)
    , delim(QChar(','))
    , encl(QChar('"'))
    , term('\n')
    , altTerm("\r\n")
{

}

CsvParser::CsvParser(const QString& source)
    :src(source)
    , pos(0)
    , delim(QChar(','))
    , encl(QChar('"'))
    , term('\n')
    , altTerm("\r\n")
{
}

CsvParser::~CsvParser()
{
}

void CsvParser::setSource(const QString& source)
{
    pos = 0;
    src = source;
}

void CsvParser::setTerminator(const QChar& terminator)
{
    term = terminator;
}

void CsvParser::setAlternativeTerminator(const QString& terminator)
{
    altTerm = terminator;
}

void CsvParser::setEncloseChar(const QChar& enclose)
{
    encl = enclose;
}

void CsvParser::setDelimiter(const QChar& delimiter)
{
    delim = delimiter;
}

void CsvParser::reset()
{
    pos = 0;
}

QStringList CsvParser::parseLineOld()
{
    QStringList resultRow;
    if ((pos >= src.size() - 1) || delim.isNull()
        || ((term.isNull()) && (altTerm.isEmpty()))
        || src.isEmpty())
        return resultRow;

    QString value;

    GenericWaitingFlag genericWaitingFlag = WaitingForValue;

    QChar first;
    QChar second;
    QString firstSecond;

    int i = pos;
    while (i < src.size()) {
        first = src.at(i);
        if (i < src.size() - 1)
            second = src.at(i + 1);
        else
            second = QChar();
        firstSecond.append(first);
        firstSecond.append(second);

        switch (genericWaitingFlag) {
            // �֦+���- ���-�-TǦ��-����
        case WaitingForValue:
            // ��T����� �-��T��-�-��TǦ��-�-T�Tɦ��� ���-�-TǦ��-���� ���-�-��
            if (first == encl) {
                // �⦦����T�T� ���+���- ���-��T�T˦-�-T�Tɦ��� ���-�-TǦ��-���� ���-�-��
                genericWaitingFlag = WaitingForEnclose;
                // ��T����� T��-���+������T¦���T�
            } else if (first == delim) {
                // �ߦ�TȦ��- ���-�-TǦ��-���� �� ���+���- �-�-�-�-���- ���-�-TǦ��-��T�
                resultRow.append(value);
                value.clear();
                // ��T����� T��-���+������T¦���T� T�T�T��-��
            } else if (first == term) {
                // �ߦ�TȦ��- ���-�-TǦ��-���� �� �-T�TŦ-�+���- ���� TƦ������-
                resultRow.append(value);
                value.clear();
                pos = (++i);
                goto exit_loop;
                // ��T����� T��-���+������T¦���T� T�T�T��-��
            } else if (!firstSecond.compare(altTerm)) {
                // �ߦ�TȦ��- ���-�-TǦ��-���� �� �-T�TŦ-�+���- ���� TƦ������-
                resultRow.append(value);
                value.clear();
                pos = (i+=2);
                goto exit_loop;
                // �ئ-�-TǦ� �+�-���-���-TϦ��- ���-�-TǦ��-����
            } else if (first!=QChar(' ')) {
                // �ߦ�TȦ��- �� �-�����+�-���- ���-�-���� T��-���+�������-��T� ���-�-TǦ��-���� ������ T�T�T��-�� T� ���-����T�T�T� �-T������- �+T�Tæ��-���- �-T�T�T���T¦��-TȦ����-T�T�
                value.append(first);
                genericWaitingFlag = WaitingForDelimiterOrTerminator;
            }
            break;
            // �֦+���- ���-�-�� T��-���+�������-��T� ���-�-TǦ��-���� ������ T�T�T��-��
        case WaitingForDelimiterOrTerminator:
            // ��T����� ���-�-�� T��-���+�������-��T� T�T�T��-��
            if (first == delim) {
                // �צ-����T�T˦-�-���- ���-�-TǦ��-���� �� �-�-TǦ��-�-���- �-�����+�-T�T� ���-�-TǦ��-����
                resultRow.append(value);
                value.clear();
                genericWaitingFlag = WaitingForValue;
                // ��T����� ���-�-�� T��-���+�������-��T� T�T�T��-�� �-T�TŦ-�+���- ���� TƦ������-
            } else if (first == term) {
                // �ߦ�TȦ��- ���-�-TǦ��-���� �� �-T�TŦ-�+���- ���� TƦ������-
                resultRow.append(value);
                value.clear();
                pos = (++i);
                goto exit_loop;
                // ��T����� ���-�-�� T��-���+�������-��T� T�T�T��-�� �-T�TŦ-�+���- ���� TƦ������-
            } else if (!firstSecond.compare(altTerm)) {
                // �ߦ�TȦ��- ���-�-TǦ��-���� �� �-T�TŦ-�+���- ���� TƦ������-
                resultRow.append(value);
                value.clear();
                pos = (i+=2);
                goto exit_loop;
                // ��T��� �-T�T¦-��Ț-�-�� ����TȦ��- �- ���-�-TǦ��-����.
            } else {
                value.append(first);
            }
            break;
            // �֦+���- ���-�-�� �-��T��-�-��TǦ��-�-T�Tɦ��� ���-�-TǦ��-����
        case WaitingForEnclose:
            // ��T����� ���-�-�� �-��T��-�-��TǦ��-�-T�Tɦ��� ���-�-TǦ��-����
            if (first == encl) {
                // ��T����� T������+T�T�Tɦ��� ���-�-�� T¦-���� �-��T��-�-��TǦ��-�-��T� ���-�-TǦ��-����, ���-�-TǦ�T� T�T¦- Tͦ�T��-�-��T��-�-�-�-����
                if (second == encl) {
                    // �Ԧ-���-���-TϦ��- ���-�-TǦ��-����, ����T���T����-�����-�-���- Tͦ�T��-�-
                    value.append(first);
                    ++i;
                    // �᦬���+T�T�Tɦ��� ���-�-�� �-�- Tͦ�T��-�-��T��-�-�-�-����
                } else {
                    // �ߦ�TȦ��- ���-�-TǦ��-���� �� �-�����+�-���- T��-���+������T¦���T� ���-�-TǦ��-���� ������ T�T�T��-�� �-���� ���-����T��� �-T������- �+T�Tæ��-���-
                    resultRow.append(value);
                    value.clear();
                    genericWaitingFlag = WaitingForDelimiterOrTerminatorWithoutWriting;
                }
                // ��T����� �-��T��-�-��TǦ�T¦���T� ��Tɦ� �-�� �-T�T�T���T¦���T�T� ��T��-T�T¦- �+�-���-���-TϦ��- ���-�-TǦ��-���� �� ���+���- ���-�-���- �-��T��-�-��TǦ�T¦���T� ���-�-TǦ��-��T�
            } else {
                value.append(first);
            }
            break;
        case WaitingForDelimiterOrTerminatorWithoutWriting:
            // ��T����� ���-�-�� T��-���+�������-��T� T�T�T��-��
            if (first == delim) {
                // �ݦ-TǦ��-�-���- �-�����+�-T�T� ���-�-TǦ��-����
                genericWaitingFlag = WaitingForValue;
            } else if (first == term) {
                pos = (++i);
                goto exit_loop;
                // ��T����� T��-���+������T¦���T� T�T�T��-��
                // ��T����� ���-�-�� T��-���+�������-��T� T�T�T��-�� �-T�TŦ-�+���- ���� TƦ������-
            } else if (!firstSecond.compare(altTerm)) {
                pos = (i+=2);
                goto exit_loop;
                // ��T��� �-T�T¦-��Ț-�-�� ����TȦ��- �- ���-�-TǦ��-����.
            }
            break;
        }

        ++i;
        ++pos;
        firstSecond.clear();
    }
    switch(genericWaitingFlag) {
    case WaitingForValue:
        resultRow.append(value);
        break;
    case WaitingForEnclose:
        resultRow.append(value);
        break;
    case WaitingForDelimiterOrTerminator:
        resultRow.append(value);
        break;
    case WaitingForDelimiterOrTerminatorWithoutWriting:
        break;
    }
    exit_loop:
    return resultRow;
}

QStringList CsvParser::parseLine(bool trimSpace)
{
    QStringList resultRow;
    if ((pos >= src.size() - 1) || delim.isNull()
        || ((term.isNull()) && (altTerm.isEmpty()))
        || src.isEmpty())
        return resultRow;

    bool isEnclosed = false;

    QString value;

    QChar first;
    QChar second;
    QString firstSecond;

    int i = pos;
    // �榬���� ��T��-TŦ-�+�- ���- �+�-�-�-T˦-
    while (i < src.size()) {
        first = src.at(i);
        if (i < src.size() - 1)
            second = src.at(i + 1);
        else
            second = QChar();

        firstSecond.append(first);
        firstSecond.append(second);

        //
        if (isEnclosed) {
            if (first == encl) {
                if (second == encl) {
                    value.append(first);
                    pos = ++i;
                } else {
                    isEnclosed = false;
                }
            } else {
                value.append(first);
            }
        } else {
            if (first == encl) {
                if (second == encl) {
                    value.append(first);
                    pos = ++i;
                } else {
                    isEnclosed = true;
                }
            } else if (first == delim) {
                if (trimSpace)
                    value = value.trimmed();
                if (!value.compare(encl))
                    value.clear();
                resultRow.append(value);
                value.clear();
            } else if (first == term) {
                if (trimSpace)
                    value = value.trimmed();
                if (!value.compare(encl))
                    value.clear();
                resultRow.append(value);
                value.clear();
                pos = (++i);
                break;
            } else if (!firstSecond.compare(altTerm)) {
                if (trimSpace)
                    value = value.trimmed();
                if (!value.compare(encl))
                    value.clear();
                resultRow.append(value);
                value.clear();
                pos = (i+=2);
                break;
            } else {
                value.append(first);
            }
        }

        pos = ++i;
        firstSecond.clear();
    }
    if (!value.isEmpty() || isEnclosed || first == delim) {
        if (trimSpace)
            value = value.trimmed();
        if (!value.compare(encl))
            value.clear();
        resultRow.append(value);
    }
    return resultRow;
}


QString toCsv(const QString &plainText, const QChar& delimiter)
{
    QString result = plainText;
    result.replace("\"", "\"\"");
    result.replace("\t", QString("\"%1 \"").arg(delimiter));
    result.replace("\n", "\"\n\"");
    result.prepend("\"");
    result.append("\"");
    return result;
}

QString toCsv(const QList<QStringList>& plainText, const QChar& delimiter)
{
    QString result;
    foreach(QStringList row, plainText) {
        foreach(QString value, row) {
            value.replace("\"", "\"\"");
            value.replace(QRegExp("(\r\n|\n)"), " ");
            result.append("\"");
            result.append(value);
            result.append("\"");
            result.append(delimiter);
        }
        result.chop(1);
        result.append("\n");
    }
    return result;
}
