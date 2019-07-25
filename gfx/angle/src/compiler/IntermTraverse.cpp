





#include "compiler/intermediate.h"


















void TIntermSymbol::traverse(TIntermTraverser* it)
{
	it->visitSymbol(this);
}

void TIntermConstantUnion::traverse(TIntermTraverser* it)
{
	it->visitConstantUnion(this);
}




void TIntermBinary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	
	
	
	if(it->preVisit)
	{
		visit = it->visitBinary(PreVisit, this);
	}
	
	
	
	
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft) 
		{
			if(right)
			{
				right->traverse(it);
			}
			
			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && left)
			{
				left->traverse(it);
			}
		}
		else
		{
			if(left)
			{
				left->traverse(it);
			}
			
			if(it->inVisit)
			{
				visit = it->visitBinary(InVisit, this);
			}

			if(visit && right)
			{
				right->traverse(it);
			}
		}

		it->decrementDepth();
	}

	
	
	
	
	if(visit && it->postVisit)
	{
		it->visitBinary(PostVisit, this);
	}
}




void TIntermUnary::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitUnary(PreVisit, this);

	if (visit) {
		it->incrementDepth();
		operand->traverse(it);
		it->decrementDepth();
	}
	
	if (visit && it->postVisit)
		it->visitUnary(PostVisit, this);
}




void TIntermAggregate::traverse(TIntermTraverser* it)
{
	bool visit = true;
	
	if(it->preVisit)
	{
		visit = it->visitAggregate(PreVisit, this);
	}
	
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft)
		{
			for(TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.front())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}
		else
		{
			for(TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
			{
				(*sit)->traverse(it);

				if(visit && it->inVisit)
				{
					if(*sit != sequence.back())
					{
						visit = it->visitAggregate(InVisit, this);
					}
				}
			}
		}
		
		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitAggregate(PostVisit, this);
	}
}




void TIntermSelection::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitSelection(PreVisit, this);
	
	if (visit) {
		it->incrementDepth();
		if (it->rightToLeft) {
			if (falseBlock)
				falseBlock->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			condition->traverse(it);
		} else {
			condition->traverse(it);
			if (trueBlock)
				trueBlock->traverse(it);
			if (falseBlock)
				falseBlock->traverse(it);
		}
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitSelection(PostVisit, this);
}




void TIntermLoop::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if(it->preVisit)
	{
		visit = it->visitLoop(PreVisit, this);
	}
	
	if(visit)
	{
		it->incrementDepth();

		if(it->rightToLeft)
		{
			if(terminal)
			{
				terminal->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(test)
			{
				test->traverse(it);
			}
		}
		else
		{
			if(test)
			{
				test->traverse(it);
			}

			if(body)
			{
				body->traverse(it);
			}

			if(terminal)
			{
				terminal->traverse(it);
			}
		}

		it->decrementDepth();
	}

	if(visit && it->postVisit)
	{
		it->visitLoop(PostVisit, this);
	}
}




void TIntermBranch::traverse(TIntermTraverser* it)
{
	bool visit = true;

	if (it->preVisit)
		visit = it->visitBranch(PreVisit, this);
	
	if (visit && expression) {
		it->incrementDepth();
		expression->traverse(it);
		it->decrementDepth();
	}

	if (visit && it->postVisit)
		it->visitBranch(PostVisit, this);
}

