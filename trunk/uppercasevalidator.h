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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with THT.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UPPERCASEVALIDATOR_H
#define UPPERCASEVALIDATOR_H

#include <QRegExpValidator>

/*
 *  Validator to convert a ticker name to the upper case
 */
class UpperCaseValidator : public QRegExpValidator
{
public:
    explicit UpperCaseValidator(QObject *parent = 0);
    virtual ~UpperCaseValidator();

    virtual QValidator::State validate(QString &input, int &pos) const;
};

#endif // UPPERCASEVALIDATOR_H
