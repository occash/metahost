#include "proto.h"

QDataStream &operator<<(QDataStream &out, const ClassMeta &classMeta)
{
	out << classMeta.dataSize;
	const uint *data = classMeta.metaObject->d.data;
	out.writeRawData((const char *)data, classMeta.dataSize * sizeof(uint));
	//Check the actual size of written bytes!!

	out << classMeta.stringSize;
	const char *string = classMeta.metaObject->d.stringdata;
	out.writeRawData(string, classMeta.stringSize);

	return out;
}

QDataStream &operator>>(QDataStream &in, ClassMeta &classMeta)
{
	if(!classMeta.metaObject)
		return in;

	quint32 dataSize;
	in >> dataSize;
	classMeta.dataSize = dataSize;
	classMeta.metaObject->d.data = new uint[dataSize];
	in.readRawData((char *)classMeta.metaObject->d.data, dataSize * sizeof(quint32));

	quint32 stringSize;
	in >> stringSize;
	classMeta.stringSize = stringSize;
	classMeta.metaObject->d.stringdata = new char[stringSize];
	in.readRawData((char *)classMeta.metaObject->d.stringdata, stringSize);

	return in;
}

QDataStream &operator<<(QDataStream &out, const ObjectMeta &objMeta)
{
	out << objMeta.classInfo.size();
	for (int i = 0; i < objMeta.classInfo.size(); ++i)
	{
		out << objMeta.classInfo.at(i);
	}

	return out;
}

QDataStream &operator>>(QDataStream &in, ObjectMeta &objMeta)
{
	int size = 0;
	in >> size;

	QStringList classNames;

	for(int i = 0; i < size; ++i)
	{
		QString cname;
		in >> cname;
		objMeta.classInfo << cname;
	}

	return in;
}