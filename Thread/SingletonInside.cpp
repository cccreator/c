//�ڲ���̬ʵ��������ģʽ
class SingletonInside
{
private:
	SingletonInside(){}
public:
	static SingletonInside* getInstance()
	{
		Lock();// not needed after C++0x
		static SingletonInside instance;
		Unlock();// not needed after C++0x
		return instance;
	}
};
