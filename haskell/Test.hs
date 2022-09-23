import Foreign.C.Types (CInt(..))
import Foreign.Ptr (FunPtr)

foreign import ccall h :: FunPtr (CInt -> CInt) -> IO ()

foreign import ccall "wrapper" mkCallback :: (CInt -> CInt) -> IO (FunPtr (CInt -> CInt))

f :: CInt -> CInt -> CInt
f x y = x + y

main = do let g = f (-5)
          g' <- mkCallback g
          h g'
