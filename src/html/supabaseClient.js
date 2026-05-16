import { createClient } from 'https://cdn.jsdelivr.net/npm/@supabase/supabase-js/+esm'

const supabaseUrl = 'https://hzucoiipjnfhnqjxtrgj.supabase.co'

const supabaseKey = 'sb_publishable_47ApWRf7T1esYfIBUWkRGg_VVbAVhp3'

export const supabase = createClient(supabaseUrl, supabaseKey)