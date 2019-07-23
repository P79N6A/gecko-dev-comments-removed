

using System;

namespace SevenZip
{
	
	
	
	class DataErrorException : ApplicationException
	{
		public DataErrorException(): base("Data Error") { }
	}

	
	
	
	class InvalidParamException : ApplicationException
	{
		public InvalidParamException(): base("Invalid Parameter") { }
	}

	public interface ICodeProgress
	{
		
		
		
		
		
		
		
		
		
		void SetProgress(Int64 inSize, Int64 outSize);
	};

	public interface ICoder
	{
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		void Code(System.IO.Stream inStream, System.IO.Stream outStream,
			Int64 inSize, Int64 outSize, ICodeProgress progress);
	};

	










	
	
	
	public enum CoderPropID
	{
		
		
		
		DefaultProp = 0,
		
		
		
		DictionarySize,
		
		
		
		UsedMemorySize,
		
		
		
		Order,
		
		
		
		BlockSize,
		
		
		
		PosStateBits,
		
		
		
		LitContextBits,
		
		
		
		LitPosBits,
		
		
		
		NumFastBytes,
		
		
		
		MatchFinder,
		
		
		
		MatchFinderCycles,
		
		
		
		NumPasses,
		
		
		
		Algorithm,
		
		
		
		NumThreads,
		
		
		
		EndMarker
	};


	public interface ISetCoderProperties
	{
		void SetCoderProperties(CoderPropID[] propIDs, object[] properties);
	};

	public interface IWriteCoderProperties
	{
		void WriteCoderProperties(System.IO.Stream outStream);
	}

	public interface ISetDecoderProperties
	{
		void SetDecoderProperties(byte[] properties);
	}
}
