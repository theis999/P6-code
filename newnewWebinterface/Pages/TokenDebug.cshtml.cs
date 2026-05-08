using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace RazorPageOidc.Pages;

[Authorize]
public class TokenDebugModel : PageModel
{
    public string? AccessToken { get; set; }
    public string? IdToken { get; set; }

    public async Task OnGetAsync()
    {
        AccessToken = await HttpContext.GetTokenAsync("access_token");
        IdToken = await HttpContext.GetTokenAsync("id_token");
    }
}