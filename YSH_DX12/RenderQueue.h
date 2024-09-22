#pragma once

/*CRender Queue dont need to think about pObject memory 해제*/
/*
 *	The Job of CRenderQueue is storing RnederItem and pushing item to CommandList 
 *
 */
class CRenderQueue
{
public:/*function*/
	CRenderQueue();
	~CRenderQueue();

	bool Initialize(class CD3D12Renderer* pRenderer, UINT MaxItemCount);
	bool Add(const RenderItem* pItem);
	UINT Process(ID3D12GraphicsCommandList* pCommandList);
	void Reset();

private:/*function*/
	void Cleanup();
	const RenderItem* Dispatch();

private: /*private*/

	CD3D12Renderer* m_pRenderer = nullptr;
	char* m_pBuffer = nullptr;

	UINT mMaxBufferSize = 0;
	UINT mAllocatedSize = 0;
	UINT mReadBufferPos = 0;

};

