/*
 *  This CSV reader is grabbed from http://www.prog.org.ru/topic_12337_0.html
 *  It is written by Filonenko Mikhail, and contains no copyright or license
 *  notice
 */

/*
 *  12.06.2013 update: Filonenko Mikhail allowed to license CsvParser
 *  under the terms of LGPL v3+
 */

/*
 * This file is part of THT.
 *
 * THT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * THT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with THT.  If not, see <http://www.gnu.org/licenses/>.
 */

/*dummy CSV reader for QT4*/
/*version 0.1*/
/*11.1.2009*/
#ifndef CSV_H
#define CSV_H

//#include "myclass_api.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>

class /*MYCLASS_API*/ CsvParser /*: public QObject*/
{
    /*Q_OBJECT*/
public:
    enum GenericWaitingFlag {
        WaitingForValue,
        WaitingForEnclose,
        WaitingForDelimiterOrTerminator,
        WaitingForDelimiterOrTerminatorWithoutWriting
    };

    CsvParser();
    CsvParser(const QString& source);
    ~CsvParser();

    void setSource(const QString& source);

    void setTerminator(const QChar& terminator);
    void setAlternativeTerminator(const QString& terminator);
    void setEncloseChar(const QChar& enclose);
    void setDelimiter(const QChar& delimiter);

    void reset();
    QStringList parseLineOld();

    QStringList parseLine(bool trimSpace = true);
private:
    QString src;
    int pos;
    QChar delim;
    QChar encl;
    QChar term;
    QString altTerm;
};

#endif // CSV_H
