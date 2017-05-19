//����ģʽ
class SingletonStatic
{
private:
	static const SingletonStatic* m_instance;
	SingletonStatic(){}
public:
	static const SingletonStatic* getInstance()
	{
		return m_instance;
	}
};
//�ⲿ��ʼ�� before invoke main
const SingletonStatic* SingletonStatic::m_instance=new SingletonStatic;