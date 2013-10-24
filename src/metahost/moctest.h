#ifndef MOCTEST_H
#define MOCTEST_H

#include <QtCore/QObject>
#include <QtCore/QDebug>

class MocTest : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString string READ getString WRITE setString NOTIFY stringChanged)
    Q_ENUMS(tenum)
    Q_FLAGS(tflags)

public:
    enum tenum {
        test1,
        test2,
        test3
    };
    enum tflag {
        tflag1,
        tflag2,
        tflag3
    };
    Q_DECLARE_FLAGS(tflags, tflag)

    Q_INVOKABLE MocTest(QObject *parent = 0);
	~MocTest();

	QString getString() const { return m_string; }
	void setString(const QString& str){ m_string = str; stringChanged(); }

	Q_INVOKABLE void test() { qDebug() << "Test invoked"; }
    Q_INVOKABLE int test_params(int i, qreal q) { qDebug() << "Test params invoked"; return 5; }

public slots:
	void slot1() { qDebug() << "slot1 invoked"; emit signal1(); emit signal3(5);}
	void slot2(const QString&) { qDebug() << "slot2 invoked"; }
	void slot3(int) { qDebug() << "slot3 invoked"; }

signals:
	void signal1();
	void signal2(const QString&);
	void signal3(int);
	void stringChanged();

private:
	QString m_string;
	QObject m_clild;

};

#endif // MOCTEST_H
